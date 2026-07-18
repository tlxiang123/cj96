#!/usr/bin/env python3
"""Restore the capacity picker to single 2/3/4 digit cards only."""

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
for capacity in (2, 3, 4):
    button = find_caption(layout, f"IrrCapacity{capacity}Button")
    position = button["position"]
    center = position["left"] + position["width"] // 2
    position["left"] = center - 34
    position["width"] = 69
    position["height"] = 69
    button["picTab"] = {
        "pic0": f"w2_capacity_{capacity}_69x69.png",
        "pic2": f"w2_capacity_{capacity}_69x69_selected.png",
    }
UI_PATH.write_bytes(encode_ftu(layout, header))
print("restored single-digit 2/3/4 capacity cards")
