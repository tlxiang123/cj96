#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, decode_ftu


KINDS = {"edittext", "radiobuttons", "textview"}


def walk(node: object, key_name: str = "", out: list[dict] | None = None) -> list[dict]:
    if out is None:
        out = []
    if isinstance(node, dict):
        kind = key_name.split("__", 1)[0] if "__" in key_name else key_name
        if kind in KINDS and ("picTab" in node or "backgroundPic" in node):
            out.append({
                "kind": kind,
                "caption": node.get("caption"),
                "id": node.get("id"),
                "position": node.get("position"),
                "picTab": node.get("picTab"),
                "backgroundPic": node.get("backgroundPic"),
                "text": node.get("text"),
            })
        for key, value in node.items():
            walk(value, key, out)
    elif isinstance(node, list):
        for value in node:
            walk(value, "radiobuttons" if isinstance(value, dict) else key_name, out)
    return out


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    report = {}
    for path in sorted(UI_DIR.glob("*.ftu")):
        items = walk(decode_ftu(path)[0])
        if items:
            report[path.name] = items
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
