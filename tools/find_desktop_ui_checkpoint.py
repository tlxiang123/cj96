#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    base = Path.home() / "Desktop"
    rows = []
    for path in sorted(base.glob("cj96_board_same_as_ide_*")):
        ui = path / "open_main_ftu_here"
        rows.append({
            "path": str(path),
            "mtime": path.stat().st_mtime,
            "main_ftu": (ui / "main.ftu").is_file(),
            "page1topset_ftu": (ui / "page1topset.ftu").is_file(),
            "resources_dir": (ui / "resources").is_dir(),
        })
    print(json.dumps(rows, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
