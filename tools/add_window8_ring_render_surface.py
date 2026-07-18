#!/usr/bin/env python3
"""Add a direct Window1 image surface for the Window8 ring artwork."""

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
window1["textview__221"] = {
    "alignment": 37,
    "caption": "Window8RingPanelImage",
    "id": 50064,
    "backgroundPic": "window8_panel.png",
    "touchable": False,
    "position": {"height": 374, "left": 400, "top": 16, "width": 581},
}
FTU.write_bytes(encode_ftu(data, header))
print("added direct Window8 ring render surface")
