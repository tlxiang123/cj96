#!/usr/bin/env python3
"""Remove the overlay that prevents GroupNameEditText from receiving touches."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"


def remove_caption(node: object, caption: str) -> bool:
    if isinstance(node, dict):
        for key, value in list(node.items()):
            if isinstance(value, dict) and value.get("caption") == caption:
                del node[key]
                return True
            if remove_caption(value, caption):
                return True
    elif isinstance(node, list):
        for value in node:
            if remove_caption(value, caption):
                return True
    return False


def main() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    removed = remove_caption(layout, "GroupNameIconOverlay")
    UI_PATH.write_bytes(encode_ftu(layout, header))
    print("removed GroupNameIconOverlay" if removed else "GroupNameIconOverlay already absent")


if __name__ == "__main__":
    main()
