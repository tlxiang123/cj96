#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    names = {"main.ftu", "page1topset.ftu", "showsysdate.ftu", "deviceList.ftu"}
    candidates: list[Path] = []
    for base in [ROOT, ROOT / "Release", ROOT / "ui", ROOT / "backup"]:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.is_file() and path.name in names and path.parent != ROOT / "ui":
                candidates.append(path)
    for path in sorted(candidates, key=lambda p: (p.name, str(p))):
        stat = path.stat()
        print(f"{path} | bytes={stat.st_size} | mtime={stat.st_mtime}")


if __name__ == "__main__":
    main()
