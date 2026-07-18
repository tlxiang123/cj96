#!/usr/bin/env python3
"""Show a representative address image in the W2 FTU preview."""

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
text_view = find_caption(layout, "TextView1")
text_view["backgroundPic"] = "w2_set_address_combined_001.png"
text_view.pop("text", None)
UI_PATH.write_bytes(encode_ftu(layout, header))
print("added default address image to TextView1")
