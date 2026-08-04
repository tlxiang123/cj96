#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
NAMES = {
    "Makefile", "makefile", "GNUmakefile",
    "build.sh", "build.bat", "CMakeLists.txt",
}


def main() -> None:
    for path in sorted(ROOT.rglob("*")):
        if path.name in NAMES:
            print(path.relative_to(ROOT))


if __name__ == "__main__":
    main()
