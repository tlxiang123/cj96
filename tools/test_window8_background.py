#!/usr/bin/env python3
"""Temporarily use a known-rendering panel to diagnose Window8 visibility."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu

path = Path(__file__).resolve().parents[1] / "ui" / "main.ftu"
data, header, _ = decode_ftu(path)


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


window8 = find(data, "Window8")
window8["visible"] = True
window8["backgroundPic"] = "window_info_panel.png"
path.write_bytes(encode_ftu(data, header))
print("Window8 diagnostic background set")
