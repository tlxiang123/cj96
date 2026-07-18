#!/usr/bin/env3
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
window1.pop("backgroundPic", None)
window1["backgroundColor"] = 13624314
for key, value in list(window1.items()):
    if isinstance(value, dict) and value.get("caption") in ("Window8RingPanelImage", "Window8RingButtonSurface"):
        del window1[key]
window8 = find(data, "Window8")
window8["backgroundPic"] = "window8_panel.png"
window8.pop("visible", None)
path.write_bytes(encode_ftu(data, header))
print("removed baked Window8 diagnostics and restored Window1 background")
