#!/usr/bin/env python3
"""Render the Window2 grid once in the window background, not per list row."""

from __future__ import annotations

import shutil
from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
RESOURCE_DIR = ROOT / "resources"
BACKUP_PATH = ROOT / "ui" / "main.ftu.before-window2-static-grid-20260716.bak"

WINDOW_SIZE = (1007, 400)
TABLE_LEFT = 19
TABLE_TOP = 18
TABLE_WIDTH = 981
HEADER_HEIGHT = 55
DATA_ITEM_HEIGHT = 54
ROW_SPACING = 5
DATA_SLOT_HEIGHT = DATA_ITEM_HEIGHT + ROW_SPACING
DATA_ROWS = 5
TABLE_HEIGHT = HEADER_HEIGHT + DATA_SLOT_HEIGHT * DATA_ROWS
CORNER_RADIUS = 12

COLUMNS = (
    ("Address", 0, 160),
    ("Name", 160, 210),
    ("Type", 370, 200),
    ("Arre", 570, 215),
    ("Status", 785, 196),
)


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    raise LookupError(caption)


def _draw_vertical_gradient(
    draw: ImageDraw.ImageDraw,
    box: tuple[int, int, int, int],
    top_color: tuple[int, int, int, int],
    bottom_color: tuple[int, int, int, int],
) -> None:
    left, top, right, bottom = box
    height = max(1, bottom - top)
    for y in range(top, bottom):
        ratio = (y - top) / max(1, height - 1)
        color = tuple(
            round(start + (end - start) * ratio)
            for start, end in zip(top_color, bottom_color)
        )
        draw.line((left, y, right - 1, y), fill=color)


def create_static_background() -> None:
    scale = 4
    width, height = WINDOW_SIZE
    canvas = Image.new("RGBA", (width * scale, height * scale), (255, 255, 255, 255))
    table = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
    table_draw = ImageDraw.Draw(table)

    left = TABLE_LEFT * scale
    top = TABLE_TOP * scale
    right = (TABLE_LEFT + TABLE_WIDTH) * scale
    header_bottom = (TABLE_TOP + HEADER_HEIGHT) * scale
    bottom = (TABLE_TOP + TABLE_HEIGHT) * scale

    _draw_vertical_gradient(
        table_draw,
        (left, top, right, header_bottom),
        (255, 255, 255, 255),
        (226, 241, 252, 255),
    )
    for row in range(DATA_ROWS):
        row_top = header_bottom + row * DATA_SLOT_HEIGHT * scale
        row_bottom = row_top + DATA_SLOT_HEIGHT * scale
        _draw_vertical_gradient(
            table_draw,
            (left, row_top, right, row_bottom),
            (255, 255, 255, 255),
            (239, 247, 253, 255),
        )

    mask = Image.new("L", canvas.size, 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.rounded_rectangle(
        (left, top, right - 1, bottom - 1),
        radius=CORNER_RADIUS * scale,
        fill=255,
    )
    canvas.alpha_composite(Image.composite(table, Image.new("RGBA", canvas.size), mask))

    grid = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
    grid_draw = ImageDraw.Draw(grid)
    line_width = scale
    grid_draw.rounded_rectangle(
        (left, top, right - 1, bottom - 1),
        radius=CORNER_RADIUS * scale,
        outline=(0, 0, 0, 255),
        width=line_width,
    )
    for _, column_left, _ in COLUMNS[1:]:
        x = (TABLE_LEFT + column_left) * scale
        grid_draw.line((x, top, x, bottom - 1), fill=(0, 0, 0, 255), width=line_width)
    for row in range(DATA_ROWS):
        y = (TABLE_TOP + HEADER_HEIGHT + row * DATA_SLOT_HEIGHT) * scale
        grid_draw.line((left, y, right - 1, y), fill=(0, 0, 0, 255), width=line_width)
    canvas.alpha_composite(grid)

    output = canvas.resize(WINDOW_SIZE, Image.Resampling.LANCZOS)
    output.save(RESOURCE_DIR / "w2_bgr.png", format="PNG", optimize=True)


def _set_columns(list_view: dict, suffix: str, height: int, top: int) -> None:
    item = list_view["item"]
    item["position"] = {"height": height + top, "left": 0, "top": 0, "width": TABLE_WIDTH}
    item.pop("picTab", None)
    item.pop("backgroundColor", None)
    for name, left, width in COLUMNS:
        subitem = find_caption(item, f"{name}{suffix}")
        subitem["position"] = {"height": height, "left": left, "top": top, "width": width}
        subitem["iconPosition"] = {"height": height, "left": 0, "top": 0, "width": width}
        subitem["colorTab"] = {"color0": 0x000000, "color1": 0x000000}


def main() -> None:
    create_static_background()

    layout, header, _ = decode_ftu(UI_PATH)
    window2 = find_caption(layout, "Window2")
    header_list = find_caption(window2, "DeviceTipListView")
    data_list = find_caption(window2, "DeviceListView")

    shutil.copy2(UI_PATH, BACKUP_PATH)
    window2.pop("backgroundColor", None)
    window2["backgroundPic"] = "w2_bgr.png"
    for key in list(window2):
        value = window2[key]
        if isinstance(value, dict) and str(value.get("caption", "")).startswith(
            "Window2CornerMask"
        ):
            del window2[key]

    header_list["position"] = {
        "height": 60,
        "left": TABLE_LEFT,
        "top": TABLE_TOP,
        "width": TABLE_WIDTH,
    }
    header_list["rowSpacing"] = ROW_SPACING
    header_list["colSpacing"] = ROW_SPACING
    _set_columns(header_list, "TipSubItem", HEADER_HEIGHT, 0)

    data_list["position"] = {
        "height": 296,
        "left": TABLE_LEFT,
        "top": TABLE_TOP + HEADER_HEIGHT,
        "width": TABLE_WIDTH,
    }
    data_list["rowSpacing"] = ROW_SPACING
    data_list["colSpacing"] = ROW_SPACING
    _set_columns(data_list, "SubItem", 52, 2)

    UI_PATH.write_bytes(encode_ftu(layout, header))
    verified, _, _ = decode_ftu(UI_PATH)
    verified_window = find_caption(verified, "Window2")
    verified_list = find_caption(verified_window, "DeviceListView")
    if verified_list.get("rowSpacing") != ROW_SPACING:
        shutil.copy2(BACKUP_PATH, UI_PATH)
        raise RuntimeError("Window2 static grid row spacing verification failed")
    if verified_list["item"].get("picTab"):
        shutil.copy2(BACKUP_PATH, UI_PATH)
        raise RuntimeError("Window2 static grid still has a row picture")
    print("Window2 static background grid applied")


if __name__ == "__main__":
    main()
