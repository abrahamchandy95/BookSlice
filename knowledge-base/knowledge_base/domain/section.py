"""Representation of a book section"""

from dataclasses import dataclass

from .types import BookTitle, Chapter


@dataclass(frozen=True)
class Section:
    """Object of a book section"""

    id: str
    book_title: BookTitle
    chapter: Chapter
    section_index: int
    content: str
