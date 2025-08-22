"""Lightweight helpers shared accross modules"""

import json
import re
from typing import Any, Dict, Iterable, List


def parse_json(s: str) -> Dict[str, Any]:
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


def extract_message_content(raw: Dict[str, Any]) -> str:
    """Get text from /api/chat or /api/generate."""
    msg = raw.get("message")
    if isinstance(msg, dict) and "content" in msg:
        return str(msg.get("content") or "")
    resp = raw.get("response")
    if isinstance(resp, str):
        return resp
    return ""


def dedup(items: Iterable[str]) -> List[str]:
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


def slugify(s: str) -> str:
    """Very small slug for filenames."""
    s = s.strip().lower()
    s = re.sub(r"[^a-z0-9]+", "-", s)
    return re.sub(r"-{2,}", "-", s).strip("-") or "book"
