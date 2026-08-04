#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, decode_ftu


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


def walk(node: object, path: str = "", out: list[dict] | None = None) -> list[dict]:
    if out is None:
        out = []
    if isinstance(node, dict):
        if "caption" in node:
            out.append({
                "path": path,
                "caption": node.get("caption"),
                "id": node.get("id"),
                "position": node.get("position"),
                "text": node.get("text"),
                "backgroundPic": node.get("backgroundPic"),
                "picTab": node.get("picTab"),
                "bgColorTab": node.get("bgColorTab"),
                "colorTab": node.get("colorTab"),
                "touchable": node.get("touchable"),
                "fontSize": node.get("fontSize"),
                "bold": node.get("bold"),
            })
        for key, value in node.items():
            walk(value, f"{path}/{key}", out)
    elif isinstance(node, list):
        for index, value in enumerate(node):
            walk(value, f"{path}[{index}]", out)
    return out


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    data, _, _ = decode_ftu(UI_DIR / "main.ftu")
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise SystemExit("Window5 not found")
    print(json.dumps(walk(window5), ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
