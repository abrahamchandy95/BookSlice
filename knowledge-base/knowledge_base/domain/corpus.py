"""Corpus wrapper for prerequisite computation"""

from dataclasses import dataclass
from typing import List, Mapping

from .concepts import ConceptCounts, ConceptData, ConceptIndex, build_index
from .section import Section
from .types import Key


@dataclass(frozen=True)
class ConceptCorpus:
    """Wrapper joining concepts with section texts"""

    concepts: ConceptData
    sections_by_key: Mapping[Key, Section]
    index: ConceptIndex

    @classmethod
    def from_concepts(
        cls, concepts: ConceptData, sections_by_key: Mapping[Key, Section]
    ) -> "ConceptCorpus":
        """
        Constructs a ConceptCorpus object from the concept extraction results
        """
        return cls(
            concepts=concepts,
            sections_by_key=sections_by_key,
            index=build_index(concepts),
        )

    def concept_counts(self) -> ConceptCounts:
        """Maps concept -> count from ConceptsData"""
        return self.concepts.concept_counts

    def count(self, concept: str) -> int:
        """counts occurance of a concept"""
        return self.concepts.count(concept)

    def get_concept_occurances(self, concept: str) -> List[Key]:
        """Gives location of a concept in the book"""
        cl = (concept or "").strip().lower()
        for canon, keys in self.index.keys_by_canon.items():
            if canon.lower() == cl:
                return list(keys)
        return []

    def get_content(self, key: Key, *, max_chars: int = 0) -> str:
        """Gets content in a certain part of the book"""
        s = self.sections_by_key.get(key)
        if not s:
            return ""
        context = (s.content or "").strip()
        return context[:max_chars] if max_chars and max_chars > 0 else context
