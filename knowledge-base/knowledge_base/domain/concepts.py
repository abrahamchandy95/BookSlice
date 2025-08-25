"""Shared data for extracted concepts"""

from dataclasses import dataclass
from typing import Dict, List, Mapping, Set

from .types import ConceptCounts, ConceptsByKey, Key


@dataclass(frozen=True)
class ConceptData:
    """Canon output of concept extraction using an LLM"""

    concepts_by_key: ConceptsByKey
    unique_concepts: Set[str]
    concept_counts: ConceptCounts
    sections_parsed: int
    errors: int = 0

    def count(self, concept: str) -> int:
        """Return frequency using lowercase lookup."""
        return int(self.concept_counts.get((concept or "").lower(), 0))


@dataclass(frozen=True)
class ConceptIndex:
    """index for a quick lookup"""

    canon_map: Mapping[str, str]
    keys_by_canon: Mapping[str, List[Key]]


def build_index(data: ConceptData) -> ConceptIndex:
    """Builds an index from ConceptsData"""
    allowed: Set[str] = {
        c.strip().lower() for c in data.unique_concepts if c and c.strip()
    }
    canon_map: Dict[str, str] = {}
    keys_by_canon: Dict[str, List[Key]] = {}

    for key in sorted(data.concepts_by_key.keys()):
        for concept in data.concepts_by_key.get(key, []) or []:
            c = (concept or "").strip()
            if not c:
                continue
            low = c.lower()
            if allowed and low not in allowed:
                continue
            if low not in canon_map:
                canon_map[low] = c
            canon = canon_map[low]
            keys_by_canon.setdefault(canon, []).append((key))
    return ConceptIndex(canon_map=canon_map, keys_by_canon=keys_by_canon)
