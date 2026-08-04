#!/usr/bin/env python3
"""Promote the UART valve result popup so it is visible on every main page."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"


def find_parent(node: object, caption: str):
    if not isinstance(node, dict):
        return None
    for key, value in node.items():
        if isinstance(value, dict):
            if value.get("caption") == caption:
                return node, key, value
            found = find_parent(value, caption)
            if found:
                return found
    return None


def main() -> None:
    layout, header, _ = decode_ftu(MAIN_FTU)
    found = find_parent(layout, "TestAdressTipsWindow")
    if not found:
        raise RuntimeError("TestAdressTipsWindow not found")

    parent, key, popup = found
    if parent is layout:
        print("TestAdressTipsWindow is already global")
        return
    if parent.get("caption") != "Window5":
        raise RuntimeError(f"unexpected popup parent: {parent.get('caption')}")

    parent_position = parent.get("position", {})
    popup_position = popup.get("position", {})
    popup_position["left"] += parent_position.get("left", 0)
    popup_position["top"] += parent_position.get("top", 0)
    del parent[key]
    if key in layout:
        raise RuntimeError(f"root key already exists: {key}")
    layout[key] = popup

    MAIN_FTU.write_bytes(encode_ftu(layout, header))
    print(f"promoted TestAdressTipsWindow as {key} at {popup_position}")


if __name__ == "__main__":
    main()
