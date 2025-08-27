"""Shared type aliases"""

from typing import Collection, Dict, List, Mapping, Set, Tuple, TypedDict

from .section import Section

# (book_title, chapter, section_index)
Key = Tuple[str, str, int]
Concept = str
ConceptCounts = Mapping[str, int]
ConceptsByKey = Mapping[Key, List[Concept]]
ConceptOccurrences = Mapping[Concept, List[Key]]
SectionConcepts = Tuple[Key, List[str]]
SectionMap = Mapping[Key, Section]

# graph types
Edge = Tuple[Concept, Concept]
ConceptColl = Collection[Concept]
PrereqMap = Mapping[Concept, ConceptColl]
EdgeList = List[Edge]
NodeId = int
IntEdge = Tuple[NodeId, NodeId]
IntEdgeList = List[IntEdge]
Adjacency = Dict[NodeId, List[NodeId]]
VertexSet = Set[NodeId]
ConceptIdMap = Dict[Concept, NodeId]
IdConceptMap = Dict[NodeId, Concept]
Degree = Dict[NodeId, int]


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


class GraphStats(TypedDict):
    """Gives meta data about the graph"""

    nodes: int
    edges: int
    foundations_count: int
    targets_count: int
    max_out_degree: int
    max_in_degree: int
    longest_prereq_chain: int
    top_by_out_degree: List[Tuple[str, int]]
    top_by_in_degree: List[Tuple[str, int]]
