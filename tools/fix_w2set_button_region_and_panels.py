#!/usr/bin/env python3
"""Place W2 action buttons in Region2 and generate exact-size AA panels."""

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU = ROOT / "ui" / "main.ftu"
RESOURCES = ROOT / "resources"
SCALE = 4


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


def pop_child(parent: dict, caption: str) -> tuple[str, dict]:
    for key, value in list(parent.items()):
        if isinstance(value, dict) and value.get("caption") == caption:
            del parent[key]
            return key, value
    raise KeyError(caption)


def make_panel(width: int, height: int, output: Path) -> None:
    size = (width * SCALE, height * SCALE)
    image = Image.new("RGB", size, (207, 227, 250))
    draw = ImageDraw.Draw(image)
    inner = Image.new("RGB", size)
    pixels = inner.load()
    top = (255, 255, 255)
    bottom = (238, 247, 253)
    for y in range(size[1]):
        ratio = y / max(1, size[1] - 1)
        color = tuple(round(top[i] + (bottom[i] - top[i]) * ratio) for i in range(3))
        for x in range(size[0]):
            pixels[x, y] = color
    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (2 * SCALE, 2 * SCALE, size[0] - 3 * SCALE, size[1] - 3 * SCALE),
        radius=14 * SCALE,
        fill=255,
    )
    image.paste(inner, (0, 0), mask)
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (2 * SCALE, 2 * SCALE, size[0] - 3 * SCALE, size[1] - 3 * SCALE),
        radius=14 * SCALE,
        outline=(22, 139, 255),
        width=2 * SCALE,
    )
    image.resize((width, height), Image.Resampling.LANCZOS).save(output, format="PNG", optimize=True)


data, header, _ = decode_ftu(FTU)
w2set = find(data, "w2set_window")
region2 = find(w2set, "W2SetRegion2Window")

ok_key, ok_button = pop_child(w2set, "W2_OkButton")
cancel_key, cancel_button = pop_child(w2set, "W2_CencelButton")
ok_button["position"] = {"height": 40, "left": 110, "top": 181, "width": 129}
cancel_button["position"] = {"height": 40, "left": 350, "top": 181, "width": 130}
region2[ok_key] = ok_button
region2[cancel_key] = cancel_button

regions = (
    ("W2SetRegion1Window", "w2set_region_1.png"),
    ("W2SetRegion2Window", "w2set_region_2.png"),
    ("W2SetRegion3Window", "w2set_region_3.png"),
)
for caption, filename in regions:
    region = find(w2set, caption)
    position = region["position"]
    make_panel(position["width"], position["height"], RESOURCES / filename)
    region["backgroundPic"] = filename
    region["touchable"] = False

FTU.write_bytes(encode_ftu(data, header))
print("moved W2 action buttons into Region2 and generated exact-size AA panels")
