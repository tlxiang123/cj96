#!/usr/bin/env python3
"""Let the existing page navigation control Window1/Window8 visibility."""

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
find(data, "Window1").pop("visible", None)
find(data, "Window8").pop("visible", None)
find(data, "Window8")["backgroundPic"] = "window8_panel.png"
FTU.write_bytes(encode_ftu(data, header))
print("restored default Window1 and Window8 visibility")
