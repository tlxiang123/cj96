#!/usr/bin/env python3
"""Bake the Window8 ring into Window1's background for reliable rendering."""

from pathlib import Path

from PIL import Image

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU = ROOT / "ui" / "main.ftu"
RESOURCES = ROOT / "resources"
OUTPUT = RESOURCES / "window1_with_window8_ring.png"


def find(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


panel = Image.new("RGB", (1007, 400), (207, 227, 250))
ring = Image.open(RESOURCES / "window8_panel.png").convert("RGB")
panel.paste(ring, (400, 16))
panel.save(OUTPUT, format="PNG", optimize=True)

data, header, _ = decode_ftu(FTU)
window1 = find(data, "Window1")
window1["backgroundPic"] = OUTPUT.name
window1.pop("backgroundColor", None)
window8 = find(data, "Window8")
window8["visible"] = False
FTU.write_bytes(encode_ftu(data, header))
print("baked Window8 ring into Window1 background")
