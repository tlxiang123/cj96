#!/usr/bin/env python3
"""Put W2 cancel on the left and association confirm on the right."""

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
region = find_caption(layout, "W2SetRegion2Window")
confirm = find_caption(region, "W2_OkButton")
cancel = find_caption(region, "W2_CencelButton")
left = min(confirm["position"]["left"], cancel["position"]["left"])
right = max(confirm["position"]["left"], cancel["position"]["left"])
cancel["position"]["left"] = left
confirm["position"]["left"] = right
UI_PATH.write_bytes(encode_ftu(layout, header))
print("swapped W2SetRegion2Window cancel and association button positions")
