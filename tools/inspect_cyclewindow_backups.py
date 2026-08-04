#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu


ROOT = Path(__file__).resolve().parents[1]


PATHS = [
    ROOT / "ui" / "main.ftu",
    ROOT / "Release" / "restore_ok_cancel_backup" / "main.ftu",
    ROOT / "Release" / "ui_debug_fix_backup_20260721_201748" / "main.ftu",
    ROOT / "Release" / "verify_runtime_push" / "main.ftu",
    ROOT / "Release" / "verify_board_same_current" / "main.ftu",
    ROOT / "backups" / "ui_full_20260720_131013" / "ui" / "main.ftu",
    ROOT / "Release" / "button15_enlarged_verify" / "main.ftu",
]


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


def collect(node: object, captions: set[str], found: list[dict]) -> None:
    if isinstance(node, dict):
        if node.get("caption") in captions:
            found.append({
                "caption": node.get("caption"),
                "position": node.get("position"),
                "backgroundPic": node.get("backgroundPic"),
                "picTab": node.get("picTab"),
                "text": node.get("text"),
            })
        for value in node.values():
            collect(value, captions, found)
    elif isinstance(node, list):
        for value in node:
            collect(value, captions, found)


def main() -> None:
    targets = {
        "CycleWindow",
        "CycleRegionLeftWindow",
        "CycleRegionRightWindow",
        "CycleOKButton",
        "CycleCancelButton",
        "Button23",
        "Button24",
        "Button31",
        "Button38",
    }
    result = {}
    for path in PATHS:
        if not path.is_file():
            continue
        data, _, _ = decode_ftu(path)
        cycle = find_caption(data, "CycleWindow")
        found: list[dict] = []
        collect(cycle, targets, found)
        result[str(path.relative_to(ROOT))] = found
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
