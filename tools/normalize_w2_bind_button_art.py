#!/usr/bin/env python3
from pathlib import Path

from PIL import Image

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
FTU = ROOT / "ui" / "main.ftu"

background = Image.new("RGB", (113, 113))
pixels = background.load()
top, bottom = (249, 252, 255), (241, 249, 255)
for y in range(113):
    ratio = y / 112
    color = tuple(round(top[i] + (bottom[i] - top[i]) * ratio) for i in range(3))
    for x in range(113):
        pixels[x, y] = color
source = Image.open(RESOURCES / "w2_set_bind_113x113.png").convert("RGB")
for y in range(113):
    for x in range(113):
        r, g, b = source.getpixel((x, y))
        if min(r, g, b) < 220 or b - r > 35 or g - r > 28:
            background.putpixel((x, y), (r, g, b))
output = RESOURCES / "w2_set_bind_113x113_norm.png"
background.save(output, format="PNG", optimize=True)


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
button = find(data, "GroupBind_Button")
button["picTab"] = {"pic0": output.name, "pic2": output.name}
FTU.write_bytes(encode_ftu(data, header))
print("normalized GroupBind_Button background without changing positions")
