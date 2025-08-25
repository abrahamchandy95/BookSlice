"""Shared type aliases"""

from typing import List, Mapping, Tuple, TypedDict

from .section import Section

# (book_title, chapter, section_index)
Key = Tuple[str, str, int]
Concept = str
ConceptCounts = Mapping[str, int]
ConceptsByKey = Mapping[Key, List[Concept]]
ConceptOccurrences = Mapping[Concept, List[Key]]
SectionConcepts = Tuple[Key, List[str]]
SectionMap = Mapping[Key, Section]
Edge = Tuple[Concept, Concept]


def create_key(s: Section) -> Key:
    """Creates a key for a section object for quick lookup"""
    return (s.book_title, s.chapter, int(s.section_index))


class PrereqRecord(TypedDict):
    """a record of a list of prerequisites for every concept generated"""

    concept: str
    noisy: bool
    prerequisites: List[str]


class PrereqMeta(TypedDict):
    """Metadata collected after the generated prerequisites"""

    counts: ConceptCounts
    rechecked: List[str]


class PrereqResult(TypedDict):
    """Final prerequisites output saved to disk"""

    prereq_by_concept: Mapping[str, PrereqRecord]
    noisy_concepts: List[str]
    edge: List[Edge]
    error: int
    meta: PrereqMeta
