#!/usr/bin/env python3
"""Restore system-time controls from MainContentWindow to the page root."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "showsysdate.ftu"


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def main() -> None:
    data, header, _ = decode_ftu(UI_PATH)
    root = find_caption(data, "Window1")
    main_key = None
    main_content = None
    for key, value in root.items():
        if isinstance(value, dict) and value.get("caption") == "MainContentWindow":
            main_key = key
            main_content = value
            break
    if main_key is None or main_content is None:
        raise RuntimeError("MainContentWindow not found")

    del root[main_key]
    restored = 0
    for key, value in main_content.items():
        if isinstance(value, dict) and value.get("caption"):
            root[key] = value
            restored += 1

    UI_PATH.write_bytes(encode_ftu(data, header))
    print(f"restored {restored} controls to Window1")


if __name__ == "__main__":
    main()
