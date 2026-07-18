#!/usr/bin/env python3
from pathlib import Path
from ftu_style import decode_ftu, encode_ftu

path = Path(__file__).resolve().parents[1] / "ui" / "main.ftu"
data, header, _ = decode_ftu(path)
for value in data.values():
    if isinstance(value, dict) and value.get("caption") == "Window8":
        value["backgroundPic"] = "window_info_panel.png"
        value["visible"] = True
path.write_bytes(encode_ftu(data, header))
print("set root Window8 diagnostic panel")
