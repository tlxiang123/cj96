#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu


def walk(node: object, captions: set[str], found: list[dict]) -> None:
    if isinstance(node, dict):
        if node.get("caption") in captions:
            found.append({
                key: node.get(key)
                for key in ("caption", "id", "position", "backgroundPic", "picTab", "text")
                if key in node
            })
        for value in node.values():
            walk(value, captions, found)
    elif isinstance(node, list):
        for value in node:
            walk(value, captions, found)


def main() -> None:
    data, _, _ = decode_ftu(Path("ui/main.ftu"))
    found: list[dict] = []
    walk(data, {
        "CycleRegionLeftWindow",
        "CycleRegionRightWindow",
        "CycleOKButton",
        "CycleCancelButton",
    }, found)
    print(json.dumps(found, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
