"""
Derives edges from saved prerequisite object.
Always stores edges in the following orientation: [concept, prerequisite]
"""

from knowledge_base.domain.types import (
    Concept,
    EdgeList,
    PrereqPerConcept,
    PrereqResult,
)
from knowledge_base.domain.utils import (
    edges_path,
    load_prereqs,
    prereqs_path,
    save_edges,
)


def build_edges(prereq_by_concept: PrereqPerConcept) -> EdgeList:
    """
    Builds edges like a list of [concept, prereq]
    """
    edge_list: EdgeList = []
    for concept, rec in prereq_by_concept.items():
        if rec.get("noisy"):
            continue
        for p in rec.get("prerequisites", []) or []:
            edge_list.append((concept, p))
    return edge_list


def _count_nodes_and_edges(edge_list: EdgeList) -> tuple[int, int]:
    nodes: set[Concept] = set()
    for a, b in edge_list:
        nodes.add(a)
        nodes.add(b)
    return len(nodes), len(edge_list)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Extract edges from concept, prerequisite data"
    )
    parser.add_argument(
        "book_title", help="Book title used to resolve pickle outputs"
    )
    args = parser.parse_args()

    pre_pkl = prereqs_path(args.book_title)
    edg_pkl = edges_path(args.book_title)

    pr: PrereqResult = load_prereqs(pre_pkl)
    prereqs_per_concept: PrereqPerConcept = pr["prereqs_per_concept"]
    edges: EdgeList = build_edges(prereqs_per_concept)

    num_nodes, num_edges = _count_nodes_and_edges(edges)
    save_edges(edges, edg_pkl)

    print("\n--- Edge Extraction Summary ---")
    print(f"Book:   {args.book_title}")
    print(f"Nodes:  {num_nodes}")
    print(f"Edges:  {num_edges}")
    print(f"Saved:  {edg_pkl}")
