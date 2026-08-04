#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys


SESSIONS = Path(r"C:\Users\Administrator\.codex\sessions")
TERMS = ("调试按钮", "显示按钮", "右边", "DebugBtn", "DisplayBtn")


def payload_text(payload: object) -> str:
    if isinstance(payload, str):
        return payload
    if not isinstance(payload, dict):
        return ""
    if "content" in payload:
        content = payload["content"]
        if isinstance(content, str):
            return content
        if isinstance(content, list):
            parts: list[str] = []
            for item in content:
                if isinstance(item, dict):
                    parts.append(str(item.get("text") or item.get("output_text") or ""))
                else:
                    parts.append(str(item))
            return "\n".join(parts)
    if "message" in payload:
        return str(payload["message"])
    return json.dumps(payload, ensure_ascii=False)


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    hits: list[dict] = []
    for path in SESSIONS.rglob("*.jsonl"):
        try:
            lines = path.read_text(encoding="utf-8", errors="ignore").splitlines()
        except Exception:
            continue
        for idx, line in enumerate(lines, 1):
            try:
                item = json.loads(line)
            except Exception:
                continue
            text = payload_text(item.get("payload"))
            if not text:
                continue
            score = sum(1 for term in TERMS if term in text)
            if score <= 0:
                continue
            if "调试按钮" in text or ("DebugBtn" in text and "DisplayBtn" in text):
                hits.append({
                    "timestamp": item.get("timestamp"),
                    "file": str(path),
                    "line": idx,
                    "type": item.get("type"),
                    "score": score,
                    "text": text[:1200],
                })
    hits.sort(key=lambda h: (str(h["timestamp"]), h["file"], h["line"]))
    for hit in hits:
        print(json.dumps(hit, ensure_ascii=False))


if __name__ == "__main__":
    main()
