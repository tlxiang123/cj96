#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys


def main() -> None:
    if len(sys.argv) != 4:
        raise SystemExit("usage: print_file_range.py PATH START END")
    path = Path(sys.argv[1])
    start = int(sys.argv[2])
    end = int(sys.argv[3])
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    for line_no in range(start, min(end, len(lines)) + 1):
        print(f"{line_no}: {lines[line_no - 1]}")


if __name__ == "__main__":
    main()
