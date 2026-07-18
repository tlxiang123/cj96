#!/usr/bin/env python3
"""Normalize W2 bottom button art against the top button background."""

from pathlib import Path

from PIL import Image

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
FTU = ROOT / "ui" / "main.ftu"


def normalize(source_name: str, output_name: str) -> None:
    background = Image.new("RGB", (113, 113))
    pixels = background.load()
    top = (249, 252, 255)
    bottom = (241, 249, 255)
    for y in range(113):
        ratio = y / 112
        color = tuple(round(top[i] + (bottom[i] - top[i]) * ratio) for i in range(3))
        for x in range(113):
            pixels[x, y] = color
    source = Image.open(RESOURCES / source_name).convert("RGB").resize((99, 107), Image.Resampling.LANCZOS)
    for y in range(source.height):
        for x in range(source.width):
            r, g, b = source.getpixel((x, y))
            # Preserve the blue ink and anti-aliased edges; replace the old
            # white/blue gradient with the shared top-button background.
            ink = min(r, g, b) < 220 or b - r > 35 or g - r > 28
            if ink:
                background.putpixel((x + 7, y + 3), (r, g, b))
    background.save(RESOURCES / output_name, format="PNG", optimize=True)


normalize("w2_set_delete_group_110x113_v2.png", "w2_set_delete_group_113x113_norm.png")
normalize("w2_set_rename_group_110x113_v3.png", "w2_set_rename_group_113x113_norm.png")

data, header, _ = decode_ftu(FTU)


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


region3 = find(data, "W2SetRegion3Window")
for caption, image in (
    ("W2_DelButton", "w2_set_delete_group_113x113_norm.png"),
    ("GroupNameButton", "w2_set_rename_group_113x113_norm.png"),
):
    button = find(region3, caption)
    button["picTab"] = {"pic0": image, "pic2": image}
    button["iconPosition"] = {"height": 113, "left": 0, "top": 0, "width": 113}
    button["position"]["width"] = 113
FTU.write_bytes(encode_ftu(data, header))
print("normalized bottom W2 button backgrounds and visual sizes")
