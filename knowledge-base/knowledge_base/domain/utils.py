"""Utilty functions"""

import os
import pickle
import re
from typing import Any

from knowledge_base.domain import ConceptData, Section
from knowledge_base.domain.types import (
    Edge,
    EdgeList,
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


def edges_path(book_title: str) -> str:
    "Path of saved edge list derived from prerequisites"
    return os.path.join(results_dir(), "edges", f"{slugify(book_title)}.pkl")


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
    if not isinstance(o, dict):
        raise TypeError(f"Unsupported prereqs pickle format at: {path}")
    prereqs_per_concept = o.get("prereqs_per_concept")
    noisy_concepts = o.get("noisy_concepts", [])
    errors = o.get("errors", 0)
    meta = o.get("meta", {})
    return {
        "prereqs_per_concept": prereqs_per_concept,
        "noisy_concepts": noisy_concepts,
        "errors": int(errors),
        "meta": meta,
    }


def save_prereqs(result: PrereqResult, path: str) -> str:
    "saves generated prerequisite object into a pickle"
    cleaned = {
        "prereqs_per_concept": result.get("prereqs_per_concept", {}),
        "noisy_concepts": result.get("noisy_concepts", []),
        "errors": int(result.get("errors", 0)),
        "meta": result.get("meta", {}),
    }
    return save_pickle(cleaned, path)


def load_edges(path: str) -> EdgeList:
    "loads the list of available edges"
    with open(path, "rb") as f:
        return pickle.load(f)


def save_edges(edges: EdgeList, path: str) -> str:
    "saves edges to a pickle"
    return save_pickle(edges, path)
