"""Utility functions to help with the data flow"""

from dataclasses import dataclass
from typing import Dict

import pandas as pd


@dataclass(frozen=True)
class Section:
    """Immutable domain object for a book section."""

    id: str
    book_title: str
    chapter: str
    title: str
    section_index: int
    content: str


def convert_sections_to_df(sections: Dict[str, Section]) -> pd.DataFrame:
    """Converts a dictionary of {section_id: Section} -> DataFrame"""
    rows = []
    for sid, s in sections.items():
        text = s.content or ""
        rows.append(
            {
                "section_id": sid,
                "book_title": s.book_title,
                "chapter": s.chapter,
                "title": s.title,
                "section_index": int(s.section_index),
                "content_len": len(text),
                "content": text,
            }
        )
    return (
        pd.DataFrame(rows)
        .sort_values(["book_title", "chapter", "section_index"], kind="stable")
        .set_index("section_id", drop=True)
    )
