#!/usr/bin/env python3
"""Add a timezone selector to the system-time page."""

from __future__ import annotations

import shutil
from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "showsysdate.ftu"
RESOURCE_DIRS = (ROOT / "resources", ROOT / "ui")
BACKUP_DIR = ROOT / "Release" / "timezone_selector_backup"

FONT = "Alibaba-PuHuiTi-Regular"
BLUE = 0x168BFF
BLUE_DARK = 0x005BBB
TEXT = 0x111318
PANEL = 0xF7FBFF
PRESSED = 0xEAF6FF
WHITE = 0xFFFFFF

SELECT_BG = "timezone_select_190x60.png"
PANEL_BG = "timezone_dropdown_204x230.png"

TIMEZONES = (
    ("UTC-8 洛杉矶", -480),
    ("UTC-5 纽约", -300),
    ("UTC+0 伦敦", 0),
    ("UTC+1 巴黎", 60),
    ("UTC+3 莫斯科", 180),
    ("UTC+5:30 印度", 330),
    ("UTC+7 曼谷", 420),
    ("UTC+8 北京", 480),
    ("UTC+9 东京", 540),
    ("UTC+10 悉尼", 600),
)


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def remove_captions(node: object, captions: set[str]) -> None:
    if isinstance(node, dict):
        for key, value in list(node.items()):
            if isinstance(value, dict) and value.get("caption") in captions:
                del node[key]
            else:
                remove_captions(value, captions)
    elif isinstance(node, list):
        node[:] = [
            value for value in node
            if not (isinstance(value, dict) and value.get("caption") in captions)
        ]
        for value in node:
            remove_captions(value, captions)


def draw_rounded(name: str, size: tuple[int, int], fill: int, outline: int) -> None:
    scale = 4
    image = Image.new("RGBA", (size[0] * scale, size[1] * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (2 * scale, 2 * scale, (size[0] - 2) * scale, (size[1] - 2) * scale),
        radius=10 * scale,
        fill=((fill >> 16) & 0xFF, (fill >> 8) & 0xFF, fill & 0xFF, 255),
        outline=((outline >> 16) & 0xFF, (outline >> 8) & 0xFF, outline & 0xFF, 255),
        width=2 * scale,
    )
    image = image.resize(size, Image.Resampling.LANCZOS)
    for directory in RESOURCE_DIRS:
        directory.mkdir(parents=True, exist_ok=True)
        image.save(directory / name, optimize=True)


def textview(caption: str, control_id: int, text: str, position: dict, font_size: int) -> dict:
    return {
        "alignment": 37,
        "caption": caption,
        "colorTab": {"color0": TEXT},
        "family": FONT,
        "fontSize": font_size,
        "id": control_id,
        "position": position,
        "text": text,
        "touchable": False,
    }


def button(caption: str, control_id: int, text: str, position: dict, font_size: int = 20) -> dict:
    return {
        "alignment": 37,
        "beepEnable": True,
        "bgColorTab": {"color0": PANEL, "color1": PRESSED, "color2": BLUE},
        "caption": caption,
        "colorTab": {"color0": BLUE_DARK, "color1": BLUE_DARK, "color2": WHITE},
        "family": FONT,
        "fontSize": font_size,
        "id": control_id,
        "position": position,
        "text": text,
    }


def main() -> None:
    BACKUP_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(UI_PATH, BACKUP_DIR / "showsysdate_before_timezone.ftu")

    draw_rounded(SELECT_BG, (190, 60), PANEL, BLUE)
    draw_rounded(PANEL_BG, (204, 230), WHITE, BLUE)

    data, header, _ = decode_ftu(UI_PATH)
    root = find_caption(data, "Window1")

    captions = {"TimezoneTitleText", "TimezoneSelectButton", "TimezoneDropdownWindow"}
    captions.update(f"TimezoneOptionButton{i + 1}" for i in range(len(TIMEZONES)))
    remove_captions(root, captions)

    root["textview__45"] = textview(
        "TimezoneTitleText",
        50270,
        "设置时区",
        {"height": 44, "left": 486, "top": 158, "width": 190},
        28,
    )
    root["button__46"] = {
        **button(
            "TimezoneSelectButton",
            20600,
            "UTC+8 北京",
            {"height": 60, "left": 486, "top": 226, "width": 190},
            22,
        ),
        "backgroundPic": SELECT_BG,
    }

    dropdown = {
        "backgroundPic": PANEL_BG,
        "beepEnable": True,
        "caption": "TimezoneDropdownWindow",
        "id": 110070,
        "position": {"height": 230, "left": 480, "top": 292, "width": 204},
        "visible": False,
    }
    for index, (label, _offset) in enumerate(TIMEZONES):
        col = index % 2
        row = index // 2
        dropdown[f"button__{48 + index}"] = button(
            f"TimezoneOptionButton{index + 1}",
            20601 + index,
            label,
            {
                "height": 40,
                "left": 8 + col * 96,
                "top": 10 + row * 43,
                "width": 92,
            },
            16,
        )
    root["window__47"] = dropdown

    UI_PATH.write_bytes(encode_ftu(data, header))

    verified, _, _ = decode_ftu(UI_PATH)
    for caption in captions:
        find_caption(verified, caption)
    print(f"updated {UI_PATH}")


if __name__ == "__main__":
    main()
