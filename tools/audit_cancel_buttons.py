#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, decode_ftu


def is_cancel_control(node: dict) -> bool:
    caption = str(node.get("caption", ""))
    text = str(node.get("text", ""))
    return (
        "Cancel" in caption
        or "Cencel" in caption
        or "取消" in text
        or "ȡ��" in text
        or "取消" in caption
    )


def walk(node: object, path: str, found: list[dict]) -> None:
    if isinstance(node, dict):
        if is_cancel_control(node):
            found.append({
                "path": path,
                "caption": node.get("caption"),
                "id": node.get("id"),
                "position": node.get("position"),
                "text": node.get("text"),
                "picTab": node.get("picTab"),
                "backgroundPic": node.get("backgroundPic"),
            })
        for key, value in node.items():
            walk(value, f"{path}/{key}", found)
    elif isinstance(node, list):
        for index, value in enumerate(node):
            walk(value, f"{path}[{index}]", found)


def main() -> None:
    all_found: dict[str, list[dict]] = {}
    for ftu in sorted(UI_DIR.glob("*.ftu")):
        data, _, _ = decode_ftu(ftu)
        found: list[dict] = []
        walk(data, "", found)
        if found:
            all_found[ftu.name] = found
    print(json.dumps(all_found, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
