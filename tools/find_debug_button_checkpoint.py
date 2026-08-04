#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu


ROOT = Path(__file__).resolve().parents[1]


def find_caption(node: object, caption: str) -> dict | None:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            found = find_caption(value, caption)
            if found is not None:
                return found
    elif isinstance(node, list):
        for value in node:
            found = find_caption(value, caption)
            if found is not None:
                return found
    return None


def main() -> None:
    paths: list[Path] = []
    for base in (ROOT / "ui", ROOT / "Release", ROOT / "backups"):
        if base.exists():
            paths.extend(base.rglob("page1topset.ftu"))

    rows = []
    for path in sorted(set(paths), key=lambda item: str(item)):
        try:
            data, _, _ = decode_ftu(path)
        except Exception:
            continue
        display = find_caption(data, "DisplayBtn")
        debug = find_caption(data, "DebugBtn")
        if not display or not debug:
            continue
        display_pos = display.get("position", {})
        debug_pos = debug.get("position", {})
        rows.append({
            "path": str(path.relative_to(ROOT)),
            "mtime": path.stat().st_mtime,
            "bytes": path.stat().st_size,
            "DisplayBtn": display_pos,
            "DebugBtn": debug_pos,
            "debug_right_of_display": (
                debug_pos.get("left", -1) > display_pos.get("left", 99999)
            ),
        })
    print(json.dumps(rows, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
