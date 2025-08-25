"""Representation of a book section"""

from dataclasses import dataclass


@dataclass(frozen=True)
class Section:
    """Object of a book section"""

    id: str
    book_title: str
    chapter: str
    section_index: int
    content: str
