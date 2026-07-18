#!/usr/bin/env python3
"""Match ChangeIrr_ListView to DeviceListView's static grid style."""

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU = ROOT / "ui" / "main.ftu"
OUTPUT = ROOT / "resources" / "w2_change_irr_list_546x184.png"
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


def build_grid() -> None:
    width, height = 546, 184
    image = Image.new("RGB", (width * SCALE, height * SCALE), (248, 252, 255))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (1 * SCALE, 1 * SCALE, width * SCALE - SCALE - 1, height * SCALE - SCALE - 1),
        radius=10 * SCALE,
        fill=(248, 252, 255),
        outline=(34, 34, 34),
        width=1 * SCALE,
    )
    for row in range(1, 4):
        y = row * 46 * SCALE
        draw.line((1 * SCALE, y, width * SCALE - 2 * SCALE, y), fill=(34, 34, 34), width=SCALE)
    image.resize((width, height), Image.Resampling.LANCZOS).save(OUTPUT, format="PNG", optimize=True)


build_grid()
data, header, _ = decode_ftu(FTU)
list_view = find(data, "ChangeIrr_ListView")
list_view["backgroundPic"] = OUTPUT.name
list_view["rows"] = 4
list_view["rowSpacing"] = 5
list_view["colSpacing"] = 5
item = list_view["item"]
item.pop("colorTab", None)
item.pop("backgroundColor", None)
item["position"] = {"height": 41, "left": 0, "top": 0, "width": 541}
subitem = find(list_view, "IrrArr_SubItem")
subitem.pop("backgroundColor", None)
subitem["colorTab"] = {"color0": 0}
subitem["fontSize"] = 24
subitem["touchable"] = False
subitem["position"] = {"height": 41, "left": 5, "top": 0, "width": 531}
FTU.write_bytes(encode_ftu(data, header))
print("styled ChangeIrr_ListView like DeviceListView")
