#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
NEEDLES = sys.argv[1:] or ["ScreenshotButton", "screenshotButton", "screenshot"]
SUFFIXES = {".cpp", ".cc", ".c", ".h", ".hpp", ".ftu", ".py", ".md"}


def main() -> None:
    for path in sorted(ROOT.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in SUFFIXES:
            continue
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except Exception:
            continue
        for line_no, line in enumerate(text.splitlines(), 1):
            if any(needle in line for needle in NEEDLES):
                print(f"{path.relative_to(ROOT)}:{line_no}: {line}")


if __name__ == "__main__":
    main()
