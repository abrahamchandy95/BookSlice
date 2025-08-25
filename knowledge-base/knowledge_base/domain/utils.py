"""Utilty functions"""

import os
import pickle
import re
from typing import Any

from knowledge_base.domain import ConceptData, Section
from knowledge_base.domain.types import (
    PrereqResult,
    SectionMap,
)


def slugify(s: str) -> str:
    """Very small slug for filenames."""
    s = s.strip().lower()
    s = re.sub(r"[^a-z0-9]+", "-", s)
    return re.sub(r"-{2,}", "-", s).strip("-") or "book"


def results_dir() -> str:
    "Path of saved pickle files"
    return os.environ.get("RESULTS_DIR", "results")


def sections_path(book_title: str) -> str:
    "Path of section objects extracted from MongoDB"
    return os.path.join(
        results_dir(), "sections", f"{slugify(book_title)}.pkl"
    )


def concepts_path(book_title: str) -> str:
    "Path of concepts pickle"
    return os.path.join(
        results_dir(), "concepts", f"{slugify(book_title)}.pkl"
    )


def prereqs_path(book_title: str) -> str:
    "Path of saved prerequisites object"
    return os.path.join(
        results_dir(), "prerequisites", f"{slugify(book_title)}.pkl"
    )


# -------- io core --------


def _ensure_dir_for(path: str) -> None:
    os.makedirs(os.path.dirname(path) or ".", exist_ok=True)


def save_pickle(obj: Any, path: str) -> str:
    "Saves output into a pickle"
    _ensure_dir_for(path)
    with open(path, "wb") as f:
        pickle.dump(obj, f, protocol=pickle.HIGHEST_PROTOCOL)
    return path


# -------- sections --------


def load_sections_map(path: str) -> SectionMap:
    "loads sections object mapped to each unique key"
    with open(path, "rb") as f:
        data = pickle.load(f)
    if not isinstance(data, dict):
        raise TypeError(f"Unsupported sections pickle format at: {path}")
    if data:
        sample_key = next(iter(data))
        if not (isinstance(sample_key, tuple) and len(sample_key) == 3):
            raise TypeError(
                "Sections pickle must be keyed."
                f"Got sample key={sample_key!r}"
            )
        # spot-check values are Section
        sample_val = data[sample_key]
        if not isinstance(sample_val, Section):
            raise TypeError("Sections pickle values must be Section objects")
    return data


def save_sections_map(sections: SectionMap, path: str) -> str:
    "saves section into a pickle"
    return save_pickle(sections, path)


# -------- concepts --------


def load_concepts(path: str) -> ConceptData:
    "loads the concepts object"
    with open(path, "rb") as f:
        o = pickle.load(f)
    if isinstance(o, ConceptData):
        return o
    if isinstance(o, dict):
        # Backcompat with earlier dict-shaped pickles
        return ConceptData(
            concepts_by_key=o.get("concepts_by_key", {}),
            unique_concepts=set(o.get("unique_concepts", [])),
            concept_counts=o.get("concept_counts", {}),
            sections_parsed=o.get(
                "sections_parsed", o.get("eligible_sections", 0)
            ),
            errors=o.get("errors", 0),
        )
    raise TypeError(f"Unsupported concepts pickle format at: {path}")


def save_concepts(cd: ConceptData, path: str) -> str:
    "Saves concept objects as pickles for later use"
    return save_pickle(cd, path)


# -------- prerequisites --------


def load_prereqs(path: str) -> PrereqResult:
    "loads the prerequisite object"
    with open(path, "rb") as f:
        o = pickle.load(f)
    # We expect a dict matching PrereqResult keys
    if isinstance(o, dict) and all(
        k in o for k in ("per_concept", "edges", "meta")
    ):
        return o
    raise TypeError(f"Unsupported prereqs pickle format at: {path}")


def save_prereqs(result: PrereqResult, path: str) -> str:
    "saves generated prerequisite object into a pickle"
    return save_pickle(result, path)
