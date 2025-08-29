"""Shared type aliases"""

# types.py
from __future__ import annotations

from typing import TYPE_CHECKING, Any, Collection, Mapping, TypedDict

if TYPE_CHECKING:
    from .section import Section

# ========= Core atoms =========
type BookTitle = str
type Chapter = str
type SectionIndex = int
type Concept = str

type Key = tuple[BookTitle, Chapter, SectionIndex]
type ConceptColl = Collection[Concept]
type ConceptCounts = Mapping[Concept, int]

# ========= Graph primitives =========
type Edge = tuple[Concept, Concept]
type EdgeList = list[Edge]


# ========= Records (TypedDicts) =========
class PrereqRecord(TypedDict):
    """A record of prerequisites for a single concept."""

    concept: Concept
    noisy: bool
    prerequisites: list[Concept]


class PrereqMeta(TypedDict):
    """Metadata collected after generating prerequisites."""

    counts: ConceptCounts
    rechecked: list[Concept]


class PrereqResult(TypedDict):
    """Final prerequisites output saved to disk."""

    prereqs_by_concept: Mapping[Concept, PrereqRecord]
    noisy_concepts: list[Concept]
    error: int
    meta: PrereqMeta


class EdgesExtracted(TypedDict, total=False):
    """Extracted edges in the form concept -> prereq."""

    edges: EdgeList
    stats: dict[str, Any]


class GraphStats(TypedDict):
    """Metadata about the graph."""

    nodes: int
    edges: int
    foundations_count: int
    targets_count: int
    max_out_degree: int
    max_in_degree: int
    longest_prereq_chain: int
    top_by_out_degree: list[tuple[Concept, int]]
    top_by_in_degree: list[tuple[Concept, int]]


# ========= Collections / mappings =========
type ConceptsByKey = Mapping[Key, list[Concept]]
type ConceptOccurrences = Mapping[Concept, list[Key]]
type SectionConcepts = tuple[Key, list[Concept]]
type SectionMap = Mapping[Key, "Section"]
type PrereqPerConcept = dict[Concept, PrereqRecord]
type PrereqMap = Mapping[Concept, ConceptColl]

type NodeId = int
type IntEdge = tuple[NodeId, NodeId]
type IntEdgeList = list[IntEdge]
type Adjacency = dict[NodeId, list[NodeId]]
type VertexSet = set[NodeId]
type ConceptIdMap = dict[Concept, NodeId]
type IdConceptMap = dict[NodeId, Concept]
type Degree = dict[NodeId, int]


# ========= Helpers =========
def create_key(s: "Section") -> Key:
    """Create a key for a Section object for quick lookup."""
    return (s.book_title, s.chapter, int(s.section_index))
