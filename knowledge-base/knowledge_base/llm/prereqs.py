"""Generate must-have prerequisites from known concepts"""

import os
import pickle
from typing import Any, Dict, List, Mapping, Optional, Tuple

from requests import exceptions as excep

from knowledge_base.domain import (
    ConceptCorpus,
    ConceptData,
    ConceptOccurrences,
    Edge,
    PrereqRecord,
    PrereqResult,
    SectionMap,
    concepts_path,
    load_concepts,
    load_sections_map,
    prereqs_path,
    sections_path,
)
from knowledge_base.llm.client import ModelParams, OllamaClient
from knowledge_base.llm.utils import (
    dedup,
    llm_jsonify,
    to_str_list,
)

SYSTEM_PROMPT_PREREQUISITES = (
    "You are an expert curriculum mapper.\n"
    "You will receive ONE target concept.\n"
    "Return ONLY a single JSON object:\n"
    '  {"concept":"...", "noisy": false, "prerequisites": ["...", "..."]}\n'
    "- 'prerequisites' are 0+ foundational, language-agnostic ideas.\n"
    "- Use concise canonical names; deduplicate.\n"
    "- No people, brands, tools, or languages; prefer underlying ideas.\n"
    "- If none, return an empty list.\n"
    "No prose; only the JSON object."
)


def _empty_concept_prereq(concept: str) -> PrereqRecord:
    return {"concept": concept, "noisy": False, "prerequisites": []}


def _build_basic_noise_filter_prompt(book_title: str, chapter: str) -> str:
    return (
        f"You are an expert curriculum mapper for '{book_title}', "
        f"chapter '{chapter}'. You will receive ONE candidate concept.\n"
        "Decide if it is a legitimate learnable concept for this context.\n"
        "Return ONLY a single JSON object:\n"
        '  {"concept":"...", "noisy": true|false, '
        '"prerequisites": ["...", "..."]}\n'
        "- 'prerequisites' are 0+ foundational ideas. Deduplicate.\n"
        "- Prefer underlying ideas over tools/languages/brands/people.\n"
        "- If uncertain, use noisy=false with empty list.\n"
        "No prose; only the JSON object."
    )


def _build_context_adept_noise_filter(book_title: str, chapter: str) -> str:
    return (
        f"You are a curriculum-mapping assistant for '{book_title}', "
        f"chapter '{chapter}'. You will receive ONE candidate concept "
        "PLUS CONTEXT from the book. Treat CONTEXT as primary evidence.\n"
        "Return ONLY a single JSON object:\n"
        '  {"concept":"...", "noisy": true|false, '
        '"prerequisites": ["...", "..."]}\n'
        "- If unrelated to CONTEXT or not teachable, set noisy:true.\n"
        "- Otherwise, set noisy:false and list prerequisites.\n"
        "- Deduplicate; do not repeat the target concept.\n"
        "No prose; only the JSON object."
    )


def _normalize_response(response: Any, concept: str) -> PrereqRecord:
    o = llm_jsonify(response)
    noisy = bool(o.get("noisy", False))
    prereqs = dedup(to_str_list(o.get("prerequisites")))
    prereqs = [p for p in prereqs if p.lower() != (concept or "").lower()]
    return {"concept": concept, "noisy": noisy, "prerequisites": prereqs}


def _select_system_prompt(
    concept: str,
    count: int,
    concept_corpus: ConceptCorpus,
) -> str:
    if count > 1:
        return SYSTEM_PROMPT_PREREQUISITES
    occ = concept_corpus.get_concept_occurances(concept)
    book_title, chapter = (occ[0][0], occ[0][1]) if occ else ("", "")
    return _build_basic_noise_filter_prompt(book_title, chapter)


def _assess_prereqs_initial(
    concept: str,
    concept_corpus: ConceptCorpus,
    local_llm: OllamaClient,
    mp: ModelParams,
) -> PrereqRecord:
    count = concept_corpus.count(concept)
    prompt = _select_system_prompt(concept, count, concept_corpus)
    text = f"CONCEPT: {concept}"
    response = local_llm.generate(prompt, text, params=mp)
    norm = _normalize_response(response, concept)
    if count > 1:
        norm["noisy"] = False
    return norm


def _assess_prereqs_with_context(
    concept: str,
    concept_corpus: ConceptCorpus,
    local_llm: OllamaClient,
    mp: ModelParams,
) -> Optional[PrereqRecord]:
    occ: ConceptOccurrences = concept_corpus.get_concept_occurances(concept)
    if not occ:
        return None
    rep_key = occ[0]
    book_title, chapter = rep_key[0], rep_key[1]
    context = concept_corpus.get_content(key=rep_key, max_chars=2000)
    if not context:
        return None
    prompt = _build_context_adept_noise_filter(book_title, chapter)
    text = f"CONCEPT: {concept}\n\nCONTEXT:\n{context}"
    response = local_llm.generate(prompt, text, params=mp)
    return _normalize_response(response, concept)


def build_concept_prereq_edges_and_filter(
    response_per_concept: Mapping[str, PrereqRecord],
) -> Tuple[List[Edge], List[str]]:
    """
    Builds a relationship [prereq -> concept] for every prerequisite
    """
    edges: List[Edge] = []
    final_noise: List[str] = []
    for c, resp in response_per_concept.items():
        if resp.get("noisy"):
            final_noise.append(c)
        for p in resp.get("prerequisites", []) or []:
            edges.append((p, c))
    return edges, final_noise


def _collect_prereqs_first_pass(
    concept_corpus: ConceptCorpus,
    local_llm: OllamaClient,
    model_params: ModelParams,
) -> Tuple[Dict[str, PrereqRecord], List[str], int]:

    prereqs_by_concept: Dict[str, PrereqRecord] = {}
    noisy_concepts: List[str] = []
    err_count = 0

    all_concepts = list(concept_corpus.index.keys_by_canon.keys())
    for idx, concept in enumerate(all_concepts):
        try:
            response = _assess_prereqs_initial(
                concept, concept_corpus, local_llm, model_params
            )
            prereqs_by_concept[concept] = response
            if response.get("noisy"):
                noisy_concepts.append(concept)
            if idx % 25 == 0:
                npre = len(response.get("prerequisites", []))
                print(
                    f"[{idx+1}/{len(all_concepts)}] {concept} -> "
                    f"noisy={response.get('noisy')} prereqs={npre}"
                )
        except (excep.RequestException, ValueError, TypeError) as e:
            err_count += 1
            prereqs_by_concept[concept] = _empty_concept_prereq(concept)
            print(f"[error] {concept}: {e}")
    return prereqs_by_concept, noisy_concepts, err_count


def _run_context_aware_noise_rechecks(
    noisy_candidates: List[str],
    concept_corpus: ConceptCorpus,
    local_llm: OllamaClient,
    model_params: ModelParams,
) -> Tuple[Dict[str, PrereqRecord], List[str], int]:

    updates: Dict[str, PrereqRecord] = {}
    corrected: List[str] = []
    errors = 0

    for jdx, concept in enumerate(noisy_candidates):
        try:
            response = _assess_prereqs_with_context(
                concept,
                concept_corpus,
                local_llm,
                mp=model_params,
            )
            if response is None:
                continue
            updates[concept] = response
            if not response.get("noisy"):
                corrected.append(concept)
            if jdx % max(1, (len(noisy_candidates) // 10 or 1)) == 0:
                npre = len(response.get("prerequisites", []))
                print(
                    f"[recheck {jdx+1}/{len(noisy_candidates)}] {concept} -> "
                    f"noisy={response.get('noisy')} prereqs={npre}"
                )
        except (excep.RequestException, ValueError, TypeError) as e:
            errors += 1
            print(f"[error in recheck] {concept}: {e}")
    return updates, corrected, errors


def get_prereqs_from_concepts(
    concept_corpus: ConceptCorpus,
    local_llm: OllamaClient,
    model_params: Optional[ModelParams] = None,
) -> PrereqResult:
    """Generates prerequisites for each concept using a two-pass approach"""

    mp = model_params or ModelParams(
        temperature=0.0, num_tokens=400, allow_thinking=False
    )
    # pass 1
    prereqs_by_concept, noisy_candidates, err1 = _collect_prereqs_first_pass(
        concept_corpus=concept_corpus,
        local_llm=local_llm,
        model_params=mp,
    )
    # pass 2
    overrides, denoised, err2 = _run_context_aware_noise_rechecks(
        noisy_candidates=noisy_candidates,
        concept_corpus=concept_corpus,
        local_llm=local_llm,
        model_params=mp,
    )
    prereqs_by_concept.update(overrides)
    edges, final_noise = build_concept_prereq_edges_and_filter(
        prereqs_by_concept
    )

    return {
        "per_concept": prereqs_by_concept,
        "noisy_concepts": final_noise,
        "edges": edges,
        "errors": err1 + err2,
        "meta": {
            "counts": dict(concept_corpus.concepts.concept_counts),
            "rechecked": denoised,
        },
    }


if __name__ == "__main__":
    import argparse

    from knowledge_base.domain import save_prereqs

    parser = argparse.ArgumentParser(
        description="Generate prerequisites per concept"
    )
    parser.add_argument(
        "book_title", help="Book title used to resolve pickles/outputs"
    )
    args = parser.parse_args()

    # Resolve input/output paths
    concepts_pkl = concepts_path(args.book_title)
    sections_pkl = sections_path(args.book_title)
    out_pkl = prereqs_path(args.book_title)

    if not os.path.exists(concepts_pkl):
        raise SystemExit(
            f"[error] Concepts pickle not found: {concepts_pkl}\n"
            "Run the concept extractor first."
        )

    concepts_data: ConceptData = load_concepts(concepts_pkl)

    sections_by_key: SectionMap = {}
    if os.path.exists(sections_pkl):
        try:
            sections_by_key = load_sections_map(sections_pkl)
        except TypeError as e:
            # Our loader raises TypeError for unexpected shapes/values
            print(
                f"[warn] Sections have unexpected type: {sections_pkl} :: {e}"
            )
            print("[info] Proceeding without context recheck.")
        except (pickle.UnpicklingError, EOFError) as e:
            print(f"[warn] Sections appear corrupted: {sections_pkl} :: {e}")
            print("[info] Proceeding without context recheck.")
        except OSError as e:
            # Filesystem/permission/read errors
            print(f"[warn] Could not read sections: {sections_pkl} :: {e}")
            print("[info] Proceeding without context recheck.")
    else:
        print(f"[info] No sections pickle at {sections_pkl};")

    # Build corpus and run
    corpus = ConceptCorpus.from_concepts(
        concepts=concepts_data, sections_by_key=sections_by_key
    )
    llm = OllamaClient()
    params = ModelParams(
        temperature=0.0, num_tokens=400, allow_thinking=False
    )

    result = get_prereqs_from_concepts(
        concept_corpus=corpus, local_llm=llm, model_params=params
    )

    # Save
    save_prereqs(result, out_pkl)

    # Summary
    print("\n--- Prerequisite Generation Summary ---")
    print(f"Book:               {args.book_title}")
    print(f"Concepts processed: {len(result.get('per_concept', {}))}")
    print(f"Edges:              {len(result.get('edges', []))}")
    print(f"Noisy concepts:     {len(result.get('noisy_concepts', []))}")
    print(f"Errors:             {result.get('errors', 0)}")
    print(f"Saved to:           {out_pkl}")
