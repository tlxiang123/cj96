#!/usr/bin/env python3
from pathlib import Path
from ftu_style import decode_ftu, encode_ftu

path = Path(__file__).resolve().parents[1] / "ui" / "main.ftu"
data, header, _ = decode_ftu(path)
for key, value in list(data.items()):
    if isinstance(value, dict) and value.get("caption") == "Window8RingPanelImage":
        del data[key]
surface = {
    "alignment": 37,
    "caption": "Window8RingPanelImage",
    "id": 50064,
    "backgroundPic": "window8_panel.png",
    "touchable": False,
    "position": {"height": 374, "left": 408, "top": 109, "width": 581},
}
data["textview__222"] = surface
path.write_bytes(encode_ftu(data, header))
print("promoted Window8 image surface to root")
