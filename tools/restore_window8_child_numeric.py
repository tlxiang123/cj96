#!/usr/bin/env python3
"""Put Window8 back into Window1 using the original numeric child key."""

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
window8 = None
for key, value in list(data.items()):
    if isinstance(value, dict) and value.get("caption") == "Window8":
        window8 = value
        del data[key]
        break
if window8 is None:
    window8 = find(window1, "Window8")
    for key, value in list(window1.items()):
        if value is window8:
            del window1[key]
            break
window8["visible"] = True
window8["backgroundPic"] = "window8_panel.png"
window8["position"] = {"height": 374, "left": 400, "top": 16, "width": 581}
window1["window__38"] = window8
data["window__220"] = data.pop("window__220", None) if "window__220" in data else None
if data.get("window__220") is None:
    data.pop("window__220", None)
FTU.write_bytes(encode_ftu(data, header))
print("restored Window8 as Window1/window__38")
