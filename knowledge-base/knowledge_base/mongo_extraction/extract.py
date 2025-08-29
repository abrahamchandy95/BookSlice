"""
This module exposes a class that reads sections out of a book stored
in MongoDB and yields lightweight payloads for downstream use.
"""

import os
import pickle
from typing import Any, Dict, List

from pymongo import ASCENDING, MongoClient
from pymongo.collection import Collection

from knowledge_base.domain import Section, slugify
from knowledge_base.domain.types import (
    BookTitle,
    SectionMap,
    create_key,
)

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

    def get_sections_for_book(self, book_title: BookTitle) -> SectionMap:
        """Retuns all the books sections for a book stored in MongoDB"""
        out: SectionMap = {}
        pipeline = self._pipeline(book_title, mode="docs")

        for doc in self._coll().aggregate(pipeline, allowDiskUse=True):
            s = self._to_section(doc)
            out[create_key(s)] = s
        return out

    def count_sections(self, book_title: BookTitle) -> int:
        """Returns number of sections in a book"""
        pipeline = self._pipeline(book_title, mode="count")
        res = list(self._coll().aggregate(pipeline))
        return int(res[0]["n"]) if res else 0

    def _coll(self) -> Collection:
        return self.client[self.db_name][self.collection]

    def _pipeline(self, book_title: BookTitle, mode: str) -> List[Dict]:
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
            section_index=int(doc.get("section_index", 0) or 0),
            content=str(doc.get("content", "") or ""),
        )


def save_sections(sections_by_id: SectionMap, out_path: str) -> str:
    """Save each section to a pickle"""
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)
    with open(out_path, "wb") as f:
        pickle.dump(sections_by_id, f, protocol=pickle.HIGHEST_PROTOCOL)
    return out_path


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Export book sections to a pickle"
    )
    parser.add_argument("book_title", help="Book Title stored in MongoDB")
    parser.add_argument(
        "--mongo-uri",
        default=os.environ.get("MONGO_URI", "mongodb://127.0.0.1:27017"),
        help="MongoDB connection",
    )
    parser.add_argument(
        "--results-dir",
        default=os.environ.get("RESULTS_DIR", "results"),
        help="Results directory (default: results)",
    )
    args = parser.parse_args()
    mongo_client = MongoClient(args.mongo_uri)
    extractor = SectionExtractor(mongo_client)

    NUM_SECTIONS = extractor.count_sections(args.book_title)
    print(f"Found ~{NUM_SECTIONS} sections for: {args.book_title}")

    sections_stored = extractor.get_sections_for_book(args.book_title)
    print(f"Fetched {len(sections_stored)} sections")

    slug = slugify(args.book_title)
    sections_dir = os.path.join(args.results_dir, "sections")
    os.makedirs(sections_dir, exist_ok=True)
    outpath = os.path.join(sections_dir, f"{slug}.pkl")
    save_sections(sections_stored, outpath)
    print(f"Saved sections pickle to: {outpath}")
