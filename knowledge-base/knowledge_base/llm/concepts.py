"""
Concept extraction over a Section object.
"""

import argparse
import json
import os
import pickle
from collections import defaultdict
from typing import Any, Dict, Iterable, List, Mapping, Optional, Set, Tuple

from pymongo import MongoClient
from requests import RequestException

from knowledge_base.data_utils import Section
from knowledge_base.llm.client import ModelParams, OllamaClient
from knowledge_base.llm.utils import (
    dedup,
    extract_message_content,
    parse_json,
    slugify,
)
from knowledge_base.mongo_extraction import SectionExtractor

SYSTEM_PROMPT = (
    "You will receive the raw text of ONE book subsection.\n"
    "Return ONLY a single JSON object with this shape:\n"
    '  {"concepts": ["...", "..."]}\n'
    "- Concepts must be language-agnostic ideas (e.g., 'Observer pattern',\n"
    "  'Encapsulation', 'Quadratic equations').\n"
    "- Do NOT include languages, tools, frameworks, brands, or people.\n"
    "- Deduplicate; use concise canonical names.\n"
    'If nothing substantive, return {"concepts": []}.\n'
    "No prose; only the JSON object."
)


def create_key(s: Section) -> Tuple[str, str, int]:
    "Creates a unique key for each section"
    return (s.book_title, s.chapter, int(s.section_index))


def _normalize_output(out: Any) -> Dict[str, list[Any]]:
    def _as_list(x: Any) -> list[Any]:
        if x is None:
            return []
        if isinstance(x, list):
            return x
        if isinstance(x, (tuple, set)):
            return list(x)
        return [x]

    def _from_json(text: str) -> Dict[str, List[Any]]:
        try:
            data = parse_json(text)
        except (json.JSONDecodeError, ValueError, TypeError):
            return {"concepts": []}

        if not isinstance(data, Mapping):
            return {"concepts": []}

        return {"concepts": _as_list(data.get("concepts"))}

    match out:
        case {"concepts": concepts}:
            return {"concepts": _as_list(concepts)}
        case dict() as d if content := extract_message_content(d):
            return _from_json(content)
        case str(content):
            return _from_json(content)
        case _:
            return {"concepts": []}


def _track_concepts(
    concepts: List[str],
    concept_counts: Dict[str, int],
    unique_concepts: Set[str],
) -> None:
    for c in concepts:
        k = (c or "").strip()
        if not k:
            continue
        kl = k.lower()
        concept_counts[kl] = concept_counts.get(kl, 0) + 1
        if concept_counts[kl] == 1:
            unique_concepts.add(k)


def _ensure_dir(path: str) -> None:
    os.makedirs(path, exist_ok=True)


def _eligible(text: str, min_chars: int) -> bool:
    return len(text or "") >= int(min_chars)


def find_section_concepts(
    s: Section, llm_client: OllamaClient, mp: ModelParams
) -> Tuple[Tuple[str, str, int], List[str]]:
    """Finds concepts discussed in a single section"""
    response = _normalize_output(
        llm_client.generate(SYSTEM_PROMPT, s.content or "", params=mp)
    )
    concepts = response.get("concepts", [])
    concepts = dedup(concepts) if isinstance(concepts, list) else []
    return create_key(s), concepts


def extract_concepts(
    sections: Iterable[Section],
    llm_client: OllamaClient,
    mp: Optional[ModelParams] = None,
) -> Dict[str, Any]:
    """
    Run concept extraction directly over Section objects.

    Returns:
      {
        "concepts_by_key": dict[
            (book_title, chapter, section_index), list[str]
        ],
        "unique_concepts": Set[str],
        "concept_counts": dict[str,int],
        "eligible_sections": int,
        "errors": int
      }
    """
    if not mp:
        mp = ModelParams(
            temperature=0.0, num_tokens=400, allow_thinking=False
        )
    concepts_by_key: Dict[Tuple[str, str, int], List[str]] = {}
    unique_concepts: Set[str] = set()
    concept_counts: Dict[str, int] = defaultdict(int)
    eligible = 0
    errors = 0

    for idx, s in enumerate(sections):
        text = s.content or ""
        if not _eligible(text, 100):
            continue
        eligible += 1
        key = create_key(s)

        try:
            key, concepts = find_section_concepts(s, llm_client, mp)
            concepts_by_key[key] = concepts
            _track_concepts(concepts, concept_counts, unique_concepts)
            if idx % 25 == 0:
                print(f"[{idx}] {len(concepts)} concept(s) @ {key}")
                print(
                    f"{len(unique_concepts)} unique_concepts generated so far!"
                )

        except (RequestException, ValueError, TypeError) as e:
            errors += 1
            concepts_by_key[key] = []
            print(f"[{key}] extraction error: {e}")
    return {
        "concepts_by_key": concepts_by_key,
        "unique_concepts": unique_concepts,
        "concept_counts": dict(concept_counts),
        "sections_parsed": eligible,
        "errors": errors,
    }


def save_concepts_to_pickle(result: Dict[str, Any], out_path: str) -> str:
    """Writes the result to a pickle file for later use"""
    _ensure_dir(os.path.dirname(out_path) or ".")
    with open(out_path, "wb") as f:
        pickle.dump(result, f, protocol=pickle.HIGHEST_PROTOCOL)
    return out_path


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="Book Concepts.")
    parser.add_argument("book_title", help="Book Title stored in MongoDB")
    args = parser.parse_args()

    slug = slugify(args.book_title)
    OUTPATH = f"{slug}_concepts.pkl"

    mongo_uri = os.environ.get("MONGO_URI", "mongodb://127.0.0.1:27017")
    results_dir = os.environ.get("RESULTS_DIR", "results")

    llm = OllamaClient()
    params = ModelParams(
        temperature=0.0, num_tokens=400, allow_thinking=False
    )

    mongo_client = MongoClient(mongo_uri)
    extractor = SectionExtractor(mongo_client)
    sections_by_id = extractor.get_sections_for_book(args.book_title)
    sections_iter = sections_by_id.values()
    print(f"Fetched {len(sections_by_id)} sections for: {args.book_title}")

    concepts_gen = extract_concepts(sections_iter, llm, mp=params)
    outfile = os.path.join(results_dir, OUTPATH)
    save_concepts_to_pickle(concepts_gen, outfile)

    print("\n--- Summary ---")
    print(f"Sections parsed: {concepts_gen['sections_parsed']}")
    print(f"Errors:          {concepts_gen['errors']}")
    print(f"Unique concepts: {len(concepts_gen['unique_concepts'])}")
    print(f"Pickle saved to: {outfile}")
