#!/usr/bin/env python3
"""Restore ThursdayButton's missing selected-state image only."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    raise LookupError(caption)


layout, header, _ = decode_ftu(UI_PATH)
button = find_caption(layout, "ThursdayButton")
button["picTab"] = {
    "pic0": "window7_select_off.png",
    "pic2": "window7_select_on.png",
}
UI_PATH.write_bytes(encode_ftu(layout, header))
print("restored ThursdayButton selected-state image")
