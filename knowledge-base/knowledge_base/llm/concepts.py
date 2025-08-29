"""
Concept extraction over a Section object.
"""

import os
from collections import defaultdict
from typing import Any, Iterable

from requests import RequestException

from knowledge_base.domain import (
    ConceptData,
    Section,
    concepts_path,
    load_sections_map,
    save_concepts,
    sections_path,
)
from knowledge_base.domain.types import (
    Concept,
    ConceptCounts,
    ConceptsByKey,
    SectionConcepts,
    SectionMap,
    create_key,
)
from knowledge_base.llm.client import ModelParams, OllamaClient
from knowledge_base.llm.utils import (
    dedup,
    llm_jsonify,
    to_str_list,
)

SYSTEM_PROMPT: str = (
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


def _normalize_output(response: Any) -> dict[str, list[Concept]]:
    """Returns {'concepts': [...]} from any kind of LLM output"""
    obj = llm_jsonify(response)
    concepts = dedup(to_str_list(obj.get("concepts")))
    return {"concepts": concepts}


def _track_concepts(
    concepts: list[Concept],
    concept_counts: ConceptCounts,
    unique_concepts: set[Concept],
) -> None:
    for c in concepts:
        k = (c or "").strip()
        if not k:
            continue
        kl = k.lower()
        concept_counts[kl] = concept_counts.get(kl, 0) + 1
        if concept_counts[kl] == 1:
            unique_concepts.add(k)


def _eligible(text: str, min_chars: int) -> bool:
    return len(text or "") >= int(min_chars)


def find_section_concepts(
    s: Section, llm_client: OllamaClient, mp: ModelParams
) -> SectionConcepts:
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
    mp: ModelParams | None = None,
) -> ConceptData:
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
    concepts_by_key: ConceptsByKey = {}
    unique_concepts: set[Concept] = set()
    concept_counts: ConceptCounts = defaultdict(int)
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

    return ConceptData(
        concepts_by_key=concepts_by_key,
        unique_concepts=unique_concepts,
        concept_counts=dict(concept_counts),
        sections_parsed=eligible,
        errors=errors,
    )


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser(
        description="Book Concepts from Section object."
    )
    parser.add_argument("book_title", help="Book Title")
    args = parser.parse_args()
    sections_pkl = sections_path(args.book_title)
    concepts_pkl = concepts_path(args.book_title)

    if not os.path.exists(sections_pkl):
        raise SystemExit(
            f"[error] Sections pickle not found: {sections_path}\n"
        )

    llm = OllamaClient()
    params = ModelParams(
        temperature=0.0, num_tokens=400, allow_thinking=False
    )

    section_map: SectionMap = load_sections_map(sections_pkl)
    print(f"Fetched {len(section_map)} sections for: {args.book_title}")

    concepts_gen = extract_concepts(section_map.values(), llm, mp=params)
    os.makedirs(os.path.dirname(concepts_pkl), exist_ok=True)
    save_concepts(concepts_gen, concepts_pkl)

    print("\n--- Summary ---")
    print(f"Sections parsed: {concepts_gen.sections_parsed}")
    print(f"Errors:          {concepts_gen.errors}")
    print(f"Unique concepts: {len(concepts_gen.unique_concepts)}")
    print(f"Pickle saved to: {concepts_pkl}")
