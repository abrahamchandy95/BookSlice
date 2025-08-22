"""
This module exposes a class that reads sections out of a book stored
in MongoDB and yields lightweight payloads for downstream use.
"""

from typing import Any, Dict, List

from pymongo import ASCENDING, MongoClient
from pymongo.collection import Collection

from knowledge_base.data_utils import Section

MIN_CHARS = 100
DB_NAME = "bookslice"
COLLECTION = "sections"

_PROJECT_DOC_FIELDS = {
    "book_title": 1,
    "chapter": 1,
    "title": 1,
    "section_index": 1,
    "content": 1,
}


class SectionExtractor:
    """Extract sections for a single book stored in MongoDB"""

    def __init__(self, client: MongoClient) -> None:
        self.client = client
        self.db_name = DB_NAME
        self.collection = COLLECTION

    def get_sections_for_book(self, book_title: str) -> Dict[str, Section]:
        """Retuns all the books sections for a book stored in MongoDB"""
        out: Dict[str, Section] = {}
        pipeline = self._pipeline(book_title, mode="docs")

        for doc in self._coll().aggregate(pipeline, allowDiskUse=True):
            s = self._to_section(doc)
            out[s.id] = s
        return out

    def count_sections(self, book_title: str) -> int:
        """Returns number of sections in a book"""
        pipeline = self._pipeline(book_title, mode="count")
        res = list(self._coll().aggregate(pipeline))
        return int(res[0]["n"]) if res else 0

    def _coll(self) -> Collection:
        return self.client[self.db_name][self.collection]

    def _pipeline(self, book_title: str, mode: str) -> List[Dict]:
        base = [
            {"$match": {"book_title": book_title}},
            {
                "$match": {
                    "$expr": {
                        "$gte": [
                            {"$strLenCP": {"$ifNull": ["$content", ""]}},
                            int(MIN_CHARS),
                        ]
                    }
                }
            },
        ]
        if mode == "docs":
            result = [
                *base,
                {"$project": _PROJECT_DOC_FIELDS},
                {"$sort": {"_id": ASCENDING}},
            ]
            return result
        if mode == "count":
            return [*base, {"$count": "n"}]
        raise ValueError("mode must be 'docs' or 'count'")

    @staticmethod
    def _to_section(doc: Dict[str, Any]) -> Section:
        return Section(
            id=str(doc.get("_id", "")),
            book_title=str(doc.get("book_title", "") or ""),
            chapter=str(doc.get("chapter", "") or ""),
            title=str(doc.get("title", "") or ""),
            section_index=int(doc.get("section_index", 0) or 0),
            content=str(doc.get("content", "") or ""),
        )
