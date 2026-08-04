#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu


ROOT = Path(__file__).resolve().parents[1]
TARGET_NAMES = {"main.ftu", "page1topset.ftu", "showsysdate.ftu", "deviceList.ftu"}


def is_target(node: dict) -> bool:
    caption = str(node.get("caption", ""))
    text = str(node.get("text", ""))
    return (
        "Cancel" in caption or "Cencel" in caption or "OK" in caption
        or "Ok" in caption or "Confirm" in caption
        or text in {"ȡ��", "ȷ��", "取消", "确定"}
    )


def walk(node: object, found: list[dict]) -> None:
    if isinstance(node, dict):
        if is_target(node):
            found.append({
                "caption": node.get("caption"),
                "id": node.get("id"),
                "position": node.get("position"),
                "text": node.get("text"),
                "picTab": node.get("picTab"),
                "backgroundPic": node.get("backgroundPic"),
                "fontSize": node.get("fontSize"),
                "colorTab": node.get("colorTab"),
            })
        for value in node.values():
            walk(value, found)
    elif isinstance(node, list):
        for value in node:
            walk(value, found)


def main() -> None:
    roots = [ROOT / "ui", ROOT / "Release", ROOT / "backups"]
    paths: list[Path] = []
    for base in roots:
        if base.exists():
            for path in base.rglob("*.ftu"):
                if path.name in TARGET_NAMES:
                    paths.append(path)
    result = {}
    for path in sorted(set(paths), key=lambda p: str(p)):
        try:
            data, _, _ = decode_ftu(path)
        except Exception as exc:
            result[str(path.relative_to(ROOT))] = f"decode error: {exc}"
            continue
        found: list[dict] = []
        walk(data, found)
        if found:
            result[str(path.relative_to(ROOT))] = found
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
