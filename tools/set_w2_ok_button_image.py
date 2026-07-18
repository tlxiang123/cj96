#!/usr/bin/env python3
"""Use the dedicated 确定 image for W2_OkButton without moving it."""

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
button = find_caption(layout, "W2_OkButton")
button["picTab"] = {"pic0": "w2_ok_120x60.png", "pic2": "w2_ok_120x60.png"}
button.pop("text", None)
UI_PATH.write_bytes(encode_ftu(layout, header))
print("changed W2_OkButton image to 确定")
