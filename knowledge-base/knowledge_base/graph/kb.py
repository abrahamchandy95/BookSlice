"""
Map all concepts to integers and add concepts and prerequisites
onto a knowledge graph.
"""

from collections import defaultdict
from functools import lru_cache
from typing import List, Tuple

from knowledge_base.domain.types import (
    Adjacency,
    Concept,
    ConceptColl,
    ConceptIdMap,
    Degree,
    EdgeList,
    GraphStats,
    IdConceptMap,
    IntEdge,
    IntEdgeList,
    NodeId,
    VertexSet,
)


class KnowledgeBase:
    """
    Graph showing concept-prerequisite relationships
    Note that an edge is always defined as [concept, prerequisite].
    For example, the pair [0, 1] indicates that in order to understand
    concept 0, you need to understand concept 1.
    """

    def __init__(self) -> None:
        self.edges: IntEdgeList = []
        self._edge_set: set[IntEdge] = set()
        self._adj: Adjacency = defaultdict(list)
        self._rev: Adjacency = defaultdict(list)

        self._id_of: ConceptIdMap = {}
        self._name_of: IdConceptMap = {}
        self._next_id: NodeId = 0

    def add(self, concept: Concept, prerequisites: ConceptColl) -> None:
        """Adds a concept collection"""
        c_name = self._norm(concept)
        if not c_name:
            return
        concept_id = self._get_id(c_name)

        for item in prerequisites:
            if not isinstance(item, str):
                continue
            prereq = self._norm(item)
            if not prereq:
                continue
            prereq_id = self._get_id(prereq)
            if concept_id == prereq_id:
                continue
            self._add_edge_ids(concept_id, prereq_id)

    def add_prerequisite(self, concept: str, prerequisite: str) -> None:
        """Adds a single prerequisite edge (concept -> prerequisite)"""
        c = self._norm(concept)
        p = self._norm(prerequisite)
        if not c or not p:
            return
        c_id = self._get_id(c)
        p_id = self._get_id(p)
        if c_id == p_id:
            return
        self._add_edge_ids(c_id, p_id)

    def add_edges(self, edges: EdgeList) -> Tuple[int, int]:
        """
        Bulk add edges into the graph
        Returns (added, skipped)
        """
        added = skipped = 0
        for concept, prereq in edges:
            c = self._norm(concept)
            p = self._norm(prereq)
            if not c or not p:
                skipped += 1
                continue
            c_id = self._get_id(c)
            p_id = self._get_id(p)
            if c_id == p_id:
                skipped += 1
                continue

            if self.add_prerequisite(concept, prereq):
                added += 1
            else:
                skipped += 1

        return added, skipped

    def prerequisites_of(
        self, concept: Concept, *, transitive: bool = True
    ) -> List[str]:
        """
        Return prerequisites for a concept.
        If transitive=True, returns all ancestors under concept->prereq edges.
        """
        c_norm = self._norm(concept)
        if not c_norm or c_norm not in self._id_of:
            return []
        start = self._id_of[c_norm]

        if not transitive:
            return [self._name_of[v] for v in self._adj.get(start, ())]

        seen: VertexSet = set()

        def dfs(u: NodeId) -> None:
            for v in self._adj.get(u, ()):
                if v not in seen:
                    seen.add(v)
                    dfs(v)

        dfs(start)
        return [self._name_of[v] for v in seen]

    def dependents_of(self, prerequisite: Concept) -> List[str]:
        """Return all concepts that require this prerequisite."""
        p_norm = self._norm(prerequisite)
        if not p_norm or p_norm not in self._id_of:
            return []
        p_id = self._id_of[p_norm]
        return [self._name_of[c] for c in self._rev.get(p_id, ())]

    def stats(self, topk: int = 5) -> GraphStats:
        """Returns basic stats"""

        n = len(self._vertices)
        m = len(self.edges)
        outdeg: Degree = {
            u: len(self._adj.get(u, ())) for u in self._vertices
        }
        indeg: Degree = {u: 0 for u in self._vertices}
        for u, vs in self._adj.items():
            for v in vs:
                indeg[v] = indeg.get(v, 0) + 1

        foundations = [u for u in self._vertices if outdeg.get(u, 0) == 0]
        targets = [u for u in self._vertices if indeg.get(u, 0) == 0]
        # cache the longest chain length

        @lru_cache(None)
        def depth(u: NodeId) -> int:
            children = self._adj.get(u, ())
            if not children:
                return 0
            return 1 + max(depth(v) for v in children)

        longest_chain = max((depth(u) for u in self._vertices), default=0)

        def top_items(d: Degree) -> List[Tuple[Concept, int]]:
            items = sorted(d.items(), key=lambda kv: kv[1], reverse=True)[
                :topk
            ]
            return [(self._name_of.get(i, str(i)), deg) for i, deg in items]

        return {
            "nodes": n,
            "edges": m,
            "foundations_count": len(foundations),
            "targets_count": len(targets),
            "max_out_degree": max(outdeg.values(), default=0),
            "max_in_degree": max(indeg.values(), default=0),
            "longest_prereq_chain": longest_chain,
            "top_by_out_degree": top_items(outdeg),
            "top_by_in_degree": top_items(indeg),
        }

    def name_of(self, node_id: NodeId) -> str:
        """concept name for an id"""
        if node_id not in self._name_of:
            raise KeyError(f"Unknown node id: {node_id}")
        return self._name_of[node_id]

    def id_of(self, name: Concept) -> NodeId:
        """id for the concept"""
        return self._get_id(self._norm(name))

    def as_list(self) -> IntEdgeList:
        """returns a list of edges"""
        return list(self.edges)

    def num_nodes(self) -> int:
        """Number of nodes"""
        return len(self._vertices)

    def num_edges(self) -> int:
        """Number of edges"""
        return len(self.edges)

    def _norm(self, s: str) -> str:
        return (s or "").strip().lower()

    def _get_id(self, concept_name: Concept) -> NodeId:
        if concept_name in self._id_of:
            return self._id_of[concept_name]
        idx = self._next_id
        self._next_id += 1
        self._id_of[concept_name] = idx
        self._name_of[idx] = concept_name
        return idx

    def _add_edge_ids(self, concept_id: NodeId, prereq_id: NodeId) -> None:
        key: IntEdge = (concept_id, prereq_id)
        if key in self._edge_set:
            return
        if self._has_path(prereq_id, concept_id):
            print(
                f"[skip] Adding edge"
                f"( {self._name_of[prereq_id]} -> {self._name_of[concept_id]})"
                f" would create a cycle; ignored"
            )
            return
        self._edge_set.add(key)
        self.edges.append([concept_id, prereq_id])
        self._adj[concept_id].append(prereq_id)
        self._rev[prereq_id].append(concept_id)

    def _ancestors_id(self, concept_id: NodeId) -> VertexSet:
        seen: VertexSet = set()

        def dfs(u: NodeId) -> None:
            for v in self._adj.get(u, ()):
                if v not in seen:
                    seen.add(v)
                    dfs(v)

        dfs(concept_id)
        return seen

    def _has_path(self, src: NodeId, dst: NodeId) -> bool:
        unvisited, visiting, visited = 0, 1, 2
        state = [unvisited] * max(self._next_id, 1)

        def dfs(u: NodeId) -> bool:
            if u == dst:
                return True
            s = state[u]
            if s == visiting:
                return False
            if s == visited:
                return False
            state[u] = visiting
            for v in self._adj.get(u, ()):
                if dfs(v):
                    return True
            state[u] = visited
            return False

        return dfs(src)

    @staticmethod
    def _is_acyclic(num_concepts: int, prerequisites: IntEdgeList) -> bool:
        g: Adjacency = defaultdict(list)
        for a, b in prerequisites:
            g[a].append(b)
        unvisited, visiting, visited = 0, 1, 2
        state = [unvisited] * num_concepts

        def dfs(node: int) -> bool:
            s = state[node]
            if s == visited:
                return True
            if s == visiting:
                return False
            state[node] = visiting

            for nei in g.get(node, ()):
                if not dfs(nei):
                    return False
            state[node] = visited
            return True

        for i in range(num_concepts):
            if state[i] == unvisited:
                if not dfs[i]:
                    return False
        return True
