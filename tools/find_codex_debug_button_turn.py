#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys


ROOTS = [
    Path(r"C:\Users\Administrator\.codex\sessions"),
    Path(r"C:\Users\Administrator\.codex\memories\rollout_summaries"),
]

KEYWORDS = [
    "调试按钮",
    "显示按钮",
    "DebugBtn",
    "DisplayBtn",
]


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    hits: list[tuple[str, int, str]] = []
    for root in ROOTS:
        if not root.exists():
            continue
        for path in root.rglob("*"):
            if not path.is_file() or path.suffix.lower() not in {".jsonl", ".md", ".txt"}:
                continue
            try:
                text = path.read_text(encoding="utf-8", errors="ignore")
            except Exception:
                continue
            if not any(keyword in text for keyword in KEYWORDS):
                continue
            lines = text.splitlines()
            for index, line in enumerate(lines, 1):
                if any(keyword in line for keyword in KEYWORDS):
                    start = max(0, index - 3)
                    end = min(len(lines), index + 2)
                    snippet = " ".join(item.strip() for item in lines[start:end])
                    if len(snippet) > 500:
                        snippet = snippet[:500]
                    hits.append((str(path), index, snippet))
    for path, line, snippet in hits:
        print(f"{path}:{line}: {snippet}")


if __name__ == "__main__":
    main()
