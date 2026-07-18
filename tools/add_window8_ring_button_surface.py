#!/usr/bin/env python3
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


window1 = find(data, "Window1")
window1["button__40"] = {
    "alignment": 37,
    "beepEnable": False,
    "caption": "Window8RingButtonSurface",
    "id": 20950,
    "picTab": {"pic0": "window8_panel.png"},
    "touchable": False,
    "iconPosition": {"height": 374, "left": 0, "top": 0, "width": 581},
    "position": {"height": 374, "left": 400, "top": 16, "width": 581},
}
path.write_bytes(encode_ftu(data, header))
print("added Window8 button image surface")
