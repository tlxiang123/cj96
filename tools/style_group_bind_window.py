#!/usr/bin/env python3
"""Restyle GroupBindValueWindow without changing its control IDs or callbacks."""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "main.ftu"
RESOURCE_DIR = ROOT / "resources"

OUTER_BG = (207, 227, 250, 255)
PANEL_TOP = (250, 253, 255, 255)
PANEL_BOTTOM = (235, 247, 255, 255)
HEADER_FILL = (229, 244, 255, 255)
ROW_FILL = 0xF6FBFF
ROW_SELECTED = 0xDCEEFF
BLUE = (0, 91, 187, 255)
LINE_BLUE = (18, 145, 235, 255)
SOFT_BLUE = (73, 174, 238, 255)
SCALE = 4


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    name = "msyhbd.ttc" if bold else "msyh.ttc"
    return ImageFont.truetype(str(Path(r"C:\Windows\Fonts") / name), size * SCALE)


def rounded_gradient(
    canvas: Image.Image,
    box: tuple[int, int, int, int],
    radius: int,
    top: tuple[int, int, int, int],
    bottom: tuple[int, int, int, int],
    outline: tuple[int, int, int, int],
    outline_width: int,
) -> None:
    left, upper, right, lower = (value * SCALE for value in box)
    width = right - left + 1
    height = lower - upper + 1
    gradient = Image.new("RGBA", (width, height))
    pixels = gradient.load()
    for y in range(height):
        ratio = y / max(1, height - 1)
        color = tuple(round(top[i] * (1.0 - ratio) + bottom[i] * ratio) for i in range(4))
        for x in range(width):
            pixels[x, y] = color

    mask = Image.new("L", (width, height), 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (0, 0, width - 1, height - 1),
        radius=radius * SCALE,
        fill=255,
    )
    canvas.paste(gradient, (left, upper), mask)
    ImageDraw.Draw(canvas).rounded_rectangle(
        (left, upper, right, lower),
        radius=radius * SCALE,
        outline=outline,
        width=outline_width * SCALE,
    )


def centered_text(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], text: str, text_font: ImageFont.FreeTypeFont) -> None:
    scaled_box = tuple(value * SCALE for value in box)
    bounds = draw.textbbox((0, 0), text, font=text_font)
    text_width = bounds[2] - bounds[0]
    text_height = bounds[3] - bounds[1]
    x = (scaled_box[0] + scaled_box[2] - text_width) // 2
    y = (scaled_box[1] + scaled_box[3] - text_height) // 2 - bounds[1]
    draw.text((x, y), text, font=text_font, fill=BLUE)


def build_window_background() -> None:
    image = Image.new("RGBA", (1007 * SCALE, 400 * SCALE), OUTER_BG)
    rounded_gradient(
        image,
        (1, 1, 1005, 398),
        18,
        PANEL_TOP,
        PANEL_BOTTOM,
        LINE_BLUE,
        3,
    )
    draw = ImageDraw.Draw(image)
    columns = (
        (24, 20, 333, 305, "\u5173\u8054\u4fe1\u606f"),
        (349, 20, 658, 305, "\u9009\u62e9\u6c34\u6cf5"),
        (674, 20, 983, 305, "\u9009\u62e9\u4f20\u611f\u5668"),
    )
    for left, upper, right, lower, label in columns:
        rounded_gradient(
            image,
            (left, upper, right, lower),
            12,
            PANEL_TOP,
            PANEL_BOTTOM,
            SOFT_BLUE,
            2,
        )
        draw.rounded_rectangle(
            ((left + 2) * SCALE, (upper + 2) * SCALE, (right - 2) * SCALE, 68 * SCALE),
            radius=10 * SCALE,
            fill=HEADER_FILL,
        )
        draw.line(
            ((left + 2) * SCALE, 68 * SCALE, (right - 2) * SCALE, 68 * SCALE),
            fill=SOFT_BLUE,
            width=2 * SCALE,
        )
        centered_text(draw, (left, 21, right, 67), label, font(24, bold=True))

    image.resize((1007, 400), Image.Resampling.LANCZOS).save(
        RESOURCE_DIR / "w2_group_bind_1007x400.png"
    )


def draw_link_icon(draw: ImageDraw.ImageDraw, pressed: bool) -> None:
    color = (0, 73, 153, 255) if pressed else BLUE
    width = 4 * SCALE
    draw.ellipse((10 * SCALE, 20 * SCALE, 31 * SCALE, 41 * SCALE), outline=color, width=width)
    draw.ellipse((27 * SCALE, 20 * SCALE, 48 * SCALE, 41 * SCALE), outline=color, width=width)
    draw.line((22 * SCALE, 30 * SCALE, 36 * SCALE, 30 * SCALE), fill=color, width=width)


def build_button(filename: str, label: str, kind: str, pressed: bool = False) -> None:
    image = Image.new("RGBA", (120 * SCALE, 60 * SCALE), PANEL_BOTTOM)
    fill_top = (226, 242, 255, 255) if pressed else PANEL_TOP
    fill_bottom = (207, 232, 251, 255) if pressed else (231, 245, 255, 255)
    rounded_gradient(image, (1, 1, 118, 58), 10, fill_top, fill_bottom, LINE_BLUE, 2)
    draw = ImageDraw.Draw(image)
    if kind == "link":
        draw_link_icon(draw, pressed)
    else:
        color = (0, 73, 153, 255) if pressed else BLUE
        draw.line((14 * SCALE, 19 * SCALE, 40 * SCALE, 43 * SCALE), fill=color, width=5 * SCALE)
        draw.line((40 * SCALE, 19 * SCALE, 14 * SCALE, 43 * SCALE), fill=color, width=5 * SCALE)
    centered_text(draw, (47, 0, 116, 59), label, font(22, bold=True))
    image.resize((120, 60), Image.Resampling.LANCZOS).save(RESOURCE_DIR / filename)


def find_control(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def position(control: dict, left: int, top: int, width: int, height: int) -> None:
    control["position"] = {"height": height, "left": left, "top": top, "width": width}


def style_list(control: dict, left: int) -> None:
    position(control, left, 76, 289, 216)
    control["rows"] = 4
    control["cols"] = 1
    control["rowSpacing"] = 0
    control["colSpacing"] = 0
    item = control["item"]
    position(item, 0, 0, 289, 54)
    item["alignment"] = 37
    item["fontSize"] = 22
    item["bold"] = True
    item["backgroundColor"] = ROW_FILL
    item["bgColorTab"] = {
        "color0": ROW_FILL,
        "color1": ROW_SELECTED,
        "color2": ROW_SELECTED,
    }
    item["colorTab"] = {
        "color0": 0x005BBB,
        "color1": 0x005BBB,
        "color2": 0x005BBB,
    }
    item["picTab"] = {
        "pic0": "window7_select_off.png",
        "pic2": "window7_select_on.png",
    }
    item["iconPosition"] = {"height": 28, "left": 10, "top": 13, "width": 28}


def patch_ftu() -> None:
    data, header, _ = decode_ftu(FTU_PATH)
    window = find_control(data, "GroupBindValueWindow")
    position(window, 8, 93, 1007, 400)
    window["backgroundPic"] = "w2_group_bind_1007x400.png"
    window.pop("backgroundColor", None)

    group_number = find_control(window, "GroupNumEditText")
    position(group_number, 42, 76, 273, 54)
    group_number["alignment"] = 36
    group_number["fontSize"] = 22
    group_number["bold"] = True
    group_number["touchable"] = False
    group_number["textType"] = 0
    group_number["bgColorTab"] = {"color0": ROW_FILL}
    group_number["colorTab"] = {"color0": 0x005BBB}

    summary_rows = (
        ("TextView5", 130),
        ("TextView7", 184),
        ("TextView6", 238),
    )
    for caption, top in summary_rows:
        summary = find_control(window, caption)
        position(summary, 42, top, 273, 54)
        summary["visible"] = True
        summary["alignment"] = 36
        summary["fontSize"] = 22
        summary["bold"] = True
        summary["touchable"] = False
        summary["bgColorTab"] = {"color0": ROW_FILL}
        summary["colorTab"] = {"color0": 0x005BBB}

    style_list(find_control(window, "SelectPumpListView"), 359)
    style_list(find_control(window, "SelectSenserListView"), 684)

    hidden = (
        "TextView4",
        "AddPumpEditText",
        "AddSenserEditText",
        "SelectPumpButton",
        "SelectSenserButton",
        "GroupDelButton",
    )
    for caption in hidden:
        find_control(window, caption)["visible"] = False

    FTU_PATH.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(FTU_PATH)
    check = find_control(decoded, "GroupBindValueWindow")
    if check.get("backgroundPic") != "w2_group_bind_1007x400.png":
        raise RuntimeError("GroupBindValueWindow FTU verification failed")


def main() -> None:
    build_window_background()
    patch_ftu()
    print("restyled GroupBindValueWindow")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise
