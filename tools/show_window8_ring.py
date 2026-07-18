#!/usr/bin/env python3
"""Make the restored Window8 ring panel explicitly visible."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU = ROOT / "ui" / "main.ftu"


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
window8 = find(data, "Window8")
window8["visible"] = True
window8["backgroundPic"] = "window8_panel.png"
FTU.write_bytes(encode_ftu(data, header))
print("Window8 ring panel set visible")
