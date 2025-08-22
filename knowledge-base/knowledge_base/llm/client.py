"""
Module to initialize LLM client.
"""

import json
import os
import random
import time
from dataclasses import dataclass, field
from typing import Any, ClassVar, Dict, List, Optional

import requests
from requests import exceptions as excep

from knowledge_base.llm.utils import extract_message_content, parse_json

OLLAMA_HOST = os.environ.get("OLLAMA_HOST", "http://localhost:11434")
OLLAMA_MODEL = os.environ.get("OLLAMA_MODEL", "deepseek-r1:32b")


@dataclass
class ModelParams:
    """Define the generation parameters here"""

    # core
    num_tokens: int = 400
    temperature: float = 0.0
    stop: Optional[List[str]] = None
    stream: bool = False
    keep_alive: str = "10m"

    # thinking
    allow_thinking: bool = False
    advanced: Dict[str, Any] = field(default_factory=dict)

    RESPONSE_FORMAT: ClassVar[str] = "json"
    THINK_OPEN: ClassVar[str] = "<think>"
    THINK_CLOSE: ClassVar[str] = "</think>"

    def build_stop_list(self) -> List[str]:
        """Builds a user defined stop list"""
        stops = list(self.stop or [])
        if not self.allow_thinking:
            if self.THINK_OPEN not in stops:
                stops.append(self.THINK_OPEN)
            if self.THINK_CLOSE not in stops:
                stops.append(self.THINK_CLOSE)
        return stops

    def build_options(self) -> Dict[str, Any]:
        """Builds Ollama Options to initialize client"""
        base = {
            "temperature": float(self.temperature),
            "num_predict": int(self.num_tokens),
            "stop": self.build_stop_list(),
        }
        whitelist = {
            "top_p",
            "top_k",
            "repeat_penalty",
            "presence_penalty",
            "frequency_penalty",
            "seed",
            "mirostat",
            "mirostat_eta",
            "mirostat_tau",
        }
        for k, v in (self.advanced or {}).items():
            if k in whitelist and v is not None:
                base[k] = v
        return base


class OllamaClient:
    """Client creation for local model with Ollama"""

    def __init__(
        self,
        host: Optional[str] = None,
        model: Optional[str] = None,
        timeout: float = 600,
    ) -> None:
        self.host = host or OLLAMA_HOST
        self.model = model or OLLAMA_MODEL
        self.timeout = float(timeout)

        self._min_intervals = 1.2
        self._max_attempts = 5
        self._backoff = 1.5

    def set_retry(
        self,
        *,
        min_intervals: Optional[float] = None,
        max_attempts: Optional[int] = None,
        backoff: Optional[float] = None,
    ) -> None:
        """Attempts a retry policy"""
        if min_intervals is not None:
            self._min_intervals = float(min_intervals)
        if max_attempts is not None:
            self._max_attempts = int(max_attempts)
        if backoff is not None:
            self._backoff = float(backoff)

    def set_model(self, model: str) -> None:
        """Switch the model"""
        self.model = model

    def ping(self) -> bool:
        """True if the server responds"""
        try:
            r = requests.get(f"{self.host}/api/tags", timeout=3.0)
            return r.ok
        except requests.RequestException:
            return False

    def models(self) -> List[str]:
        """Return available model names (if endpoint is supported)."""
        try:
            r = requests.get(f"{self.host}/api/tags", timeout=5.0)
            r.raise_for_status()
            data = r.json() or {}
            models = data.get("models", [])
            return [m.get("name", "") for m in models if isinstance(m, dict)]
        except requests.RequestException:
            return []

    def generate(
        self,
        system_prompt: str,
        user_text: str,
        params: Optional[ModelParams] = None,
        *,
        full_response: bool = False,
    ) -> Dict[str, Any]:
        """Calls the llm model and returns raw json from server"""
        mp = params or ModelParams()
        payload = self._build_payload(system_prompt, user_text, mp)
        response = self._send_request(payload)
        if full_response:
            return response
        content = extract_message_content(response) or "{}"
        return parse_json(content)

    def _sleep_backoff(self, b: float) -> float:
        time.sleep(b + random.uniform(0, 0.5))
        return b * 1.7

    def _build_payload(
        self, system_prompt: str, user_text: str, mp: ModelParams
    ) -> Dict[str, Any]:
        return {
            "model": self.model,
            "messages": [
                {"role": "system", "content": system_prompt},
                {"role": "user", "content": user_text},
            ],
            "stream": bool(mp.stream),
            "format": mp.RESPONSE_FORMAT,
            "keep_alive": mp.keep_alive,
            "options": mp.build_options(),
        }

    def _send_request(self, payload: Dict[str, Any]) -> Dict[str, Any]:
        attempt = 0
        last_start = 0.0
        backoff = self._backoff
        url = f"{self.host}/api/chat"

        while True:
            dt = time.time() - last_start
            if dt < self._min_intervals:
                time.sleep(self._min_intervals - dt)

            try:
                last_start = time.time()
                r = requests.post(url, json=payload, timeout=self.timeout)
                r.raise_for_status()
                return r.json()

            except (excep.ConnectionError, excep.Timeout) as e:
                attempt += 1
                if attempt >= self._max_attempts:
                    raise e
                backoff = self._sleep_backoff(backoff)

            except excep.HTTPError as e:
                attempt += 1
                resp = getattr(e, "response", None)
                status = getattr(resp, "status_code", None)
                retryable = isinstance(status, int) and 500 <= status < 600
                if attempt >= self._max_attempts or not retryable:
                    raise e
                backoff = self._sleep_backoff(backoff)

            except json.JSONDecodeError:
                return {}
