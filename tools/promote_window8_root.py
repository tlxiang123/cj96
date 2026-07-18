#!/usr/bin/env python3
"""Render Window8 as a root window so the target draws the ring panel."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU = ROOT / "ui" / "main.ftu"


def pop_caption(parent: dict, caption: str) -> dict:
    for key, value in list(parent.items()):
        if isinstance(value, dict) and value.get("caption") == caption:
            del parent[key]
            return value
    raise KeyError(caption)


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


data, header, _ = decode_ftu(FTU)
window1 = find(data, "Window1")
window8 = pop_caption(window1, "Window8")
window8["visible"] = True
window8["backgroundPic"] = "window8_panel.png"
window8["position"] = {"height": 374, "left": 408, "top": 109, "width": 581}
for key, value in list(data.items()):
    if isinstance(value, dict) and value.get("caption") == "Window8":
        del data[key]
data["window__window8_root"] = window8
FTU.write_bytes(encode_ftu(data, header))
print("promoted Window8 to root layer")
