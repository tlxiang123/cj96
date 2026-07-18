#!/usr/bin/env python3
"""Remove temporary Window8 diagnostic surfaces and keep the original child."""

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
window1 = find(data, "Window1")
for key, value in list(data.items()):
    if isinstance(value, dict) and value.get("caption") == "Window8RingPanelImage":
        del data[key]
for key, value in list(window1.items()):
    if isinstance(value, dict) and value.get("caption") == "Window8RingPanelImage":
        del window1[key]
window8 = find(data, "Window8")
window8["backgroundPic"] = "window8_panel.png"
window8.pop("visible", None)
window1.pop("visible", None)
FTU.write_bytes(encode_ftu(data, header))
print("cleaned Window8 diagnostics")
