"""Lightweight helpers shared accross modules"""

import json
from typing import Any, Iterable


def parse_json(s: str) -> dict[str, Any]:
    """fix json if malformed"""
    try:
        return json.loads(s)
    except json.JSONDecodeError:
        i, j = s.find("{"), s.rfind("}")
        if i != -1 and j != -1 and j > i:
            try:
                return json.loads(s[i : j + 1])
            except json.JSONDecodeError:
                pass
        return {}


def extract_message_content(raw: dict[str, Any]) -> str:
    """Get text from /api/chat or /api/generate."""
    msg = raw.get("message")
    if isinstance(msg, dict) and "content" in msg:
        return str(msg.get("content") or "")
    resp = raw.get("response")
    if isinstance(resp, str):
        return resp
    return ""


def dedup(items: Iterable[str]) -> list[str]:
    """Dedup and preserve first seen"""
    out, seen = [], set()
    for s in items or []:
        k = (s or "").strip()
        if not k:
            continue
        kl = k.lower()
        if kl not in seen:
            seen.add(kl)
            out.append(k)
    return out


def to_str_list(x: Any) -> list[str]:
    """Coerse into a clean list of strings"""
    if x is None:
        return []
    if isinstance(x, list):
        src = x
    elif isinstance(x, (tuple, set)):
        src = list(x)
    else:
        src = [x]
    out: List[str] = []
    for v in src:
        s = str(v).strip()
        if s:
            out.append(s)
    return out


def llm_jsonify(response: Any) -> dict[str, Any]:
    """Unifies llm outputs into a parsed json"""
    if isinstance(response, dict):
        if any(k in response for k in ("concepts", "prerequisites", "noisy")):
            return response
        content = extract_message_content(response)
        return parse_json(content) if content else {}
    if isinstance(response, str):
        return parse_json(response)
    return {}
