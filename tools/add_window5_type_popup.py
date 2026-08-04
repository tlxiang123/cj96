#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Add the Window5 decoder subtype selection popup on Window10."""

from __future__ import annotations

import re
import shutil
import sys
import time
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, RESOURCE_DIR, decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
FONT_PATH = ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf"
BACKUP_DIR = ROOT / "Release" / f"before_window5_type_popup_{time.strftime('%Y%m%d_%H%M%S')}"

BLUE = (0, 128, 255, 238)
BLUE_DARK = (0, 93, 180, 255)
WHITE = (255, 255, 255, 255)
SOFT = (245, 251, 255, 255)
FONT_FAMILY = "Alibaba-PuHuiTi-Regular"

POPUP_CAPTIONS = {
    "Window5TypePopupTitleText",
    "Window5TypeRainButton",
    "Window5TypeHumidityButton",
    "Window5TypePressureButton",
    "Window5TypeFlowButton",
    "Window5TypeACValveButton",
    "Window5TypeDCValveButton",
    "Window5TypeSensorTouchButton",
    "Window5TypeValveTouchButton",
}

BUTTONS = {
    "Window5TypeRainButton": (20092, "window5_type_rain_150x65.png", "雨量", "filter_icon_48.png"),
    "Window5TypeHumidityButton": (20093, "window5_type_humidity_150x65.png", "湿度", "water_icon_48.png"),
    "Window5TypePressureButton": (20094, "window5_type_pressure_150x65.png", "水压", "window6_pressure_icon.png"),
    "Window5TypeFlowButton": (20095, "window5_type_flow_150x65.png", "流量", "window6_flow_icon.png"),
    "Window5TypeACValveButton": (20096, "window5_type_ac_valve_150x65.png", "AC电磁阀", None),
    "Window5TypeDCValveButton": (20097, "window5_type_dc_valve_150x65.png", "DC电磁阀", None),
}


def font(size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(FONT_PATH), size)


def draw_button_base() -> Image.Image:
    scale = 4
    w, h = 150, 65
    image = Image.new("RGBA", (w * scale, h * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    def p(v: int | float) -> int:
        return int(round(v * scale))

    rect = (p(3), p(2), p(w - 4), p(h - 3))
    draw.rounded_rectangle(rect, radius=p(12), fill=SOFT, outline=BLUE, width=p(2))
    draw.rounded_rectangle((p(8), p(7), p(w - 9), p(h - 8)), radius=p(9),
                           outline=(255, 255, 255, 68), width=p(1))
    return image.resize((w, h), Image.Resampling.LANCZOS)


def paste_source_icon(image: Image.Image, source_name: str) -> None:
    source = Image.open(RESOURCE_DIR / source_name).convert("RGBA")
    source.thumbnail((44, 44), Image.Resampling.LANCZOS)
    x = 22 + (44 - source.width) // 2
    y = 10 + (44 - source.height) // 2
    image.alpha_composite(source, (x, y))


def paste_solenoid_icon(image: Image.Image) -> None:
    source = Image.open(RESOURCE_DIR / "Solenoid_Valve.png").convert("RGBA")
    icon = source.crop((6, 8, 58, 52))
    icon.thumbnail((48, 44), Image.Resampling.LANCZOS)
    image.alpha_composite(icon, (18, 10))


def make_button(path: Path, label: str, source_icon: str | None) -> None:
    image = draw_button_base()
    draw = ImageDraw.Draw(image)
    if source_icon:
        paste_source_icon(image, source_icon)
    else:
        paste_solenoid_icon(image)

    label_font = font(22 if len(label) <= 2 else 17)
    bbox = draw.textbbox((0, 0), label, font=label_font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]
    text_x = 78 if len(label) <= 2 else 64
    if text_x + text_w > 143:
        text_x = 143 - text_w
    text_y = 32 - text_h // 2 - bbox[1]
    draw.text((text_x, text_y), label, font=label_font, fill=BLUE_DARK)
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)


def sync_resource(name: str) -> None:
    source = RESOURCE_DIR / name
    target = UI_DIR / name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, target)


def find_caption(node: object, caption: str) -> dict | None:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            found = find_caption(value, caption)
            if found is not None:
                return found
    elif isinstance(node, list):
        for value in node:
            found = find_caption(value, caption)
            if found is not None:
                return found
    return None


def max_numeric_suffix(node: object, prefix: str) -> int:
    value = 0
    if isinstance(node, dict):
        for key, child in node.items():
            match = re.fullmatch(re.escape(prefix) + r"__(\d+)", key)
            if match:
                value = max(value, int(match.group(1)))
            value = max(value, max_numeric_suffix(child, prefix))
    elif isinstance(node, list):
        for child in node:
            value = max(value, max_numeric_suffix(child, prefix))
    return value


def next_key(root: dict, parent: dict, prefix: str) -> str:
    value = max_numeric_suffix(root, prefix) + 1
    while f"{prefix}__{value}" in parent:
        value += 1
    return f"{prefix}__{value}"


def remove_popup_children(window10: dict) -> None:
    for key, value in list(window10.items()):
        if isinstance(value, dict) and value.get("caption") in POPUP_CAPTIONS:
            del window10[key]


def add_button(root: dict, parent: dict, caption: str, position: dict) -> None:
    control_id, pic, _label, _source = BUTTONS[caption]
    parent[next_key(root, parent, "button")] = {
        "caption": caption,
        "id": control_id,
        "position": position,
        "visible": False,
        "touchable": True,
        "beepEnable": True,
        "picTab": {"pic0": pic, "pic1": pic, "pic2": pic},
        "iconPosition": {"left": 14, "top": 8, "width": 150, "height": 65},
    }


def make_touch_button(caption: str, control_id: int, position: dict) -> dict:
    node = {
        "caption": caption,
        "id": control_id,
        "position": position,
        "visible": True,
        "touchable": True,
        "beepEnable": True,
        "text": "",
        "bgColorTab": {"color0": -1, "color1": -1, "color2": -1},
    }
    return node


def apply_ftu() -> None:
    path = UI_DIR / "main.ftu"
    data, header, _ = decode_ftu(path)
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found")
    window10 = find_caption(data, "Window10")
    if window10 is None:
        raise RuntimeError("Window10 not found")

    remove_popup_children(window10)
    for key, value in list(window5.items()):
        if isinstance(value, dict) and value.get("caption") in {
            "Window5TypeSensorTouchButton",
            "Window5TypeValveTouchButton",
        }:
            del window5[key]

    touch_key_start = max_numeric_suffix(data, "button") + 1
    while f"button__{touch_key_start}" in window5:
        touch_key_start += 1
    sensor_key = f"button__{touch_key_start}"
    valve_key = f"button__{touch_key_start + 1}"
    sensor_touch = make_touch_button(
        "Window5TypeSensorTouchButton",
        20098,
        {"left": 66, "top": 264, "width": 154, "height": 78},
    )
    valve_touch = make_touch_button(
        "Window5TypeValveTouchButton",
        20099,
        {"left": 255, "top": 264, "width": 190, "height": 78},
    )
    rebuilt: list[tuple[str, object]] = []
    inserted = False
    for key, value in window5.items():
        if value is window10 and not inserted:
            rebuilt.append((sensor_key, sensor_touch))
            rebuilt.append((valve_key, valve_touch))
            inserted = True
        rebuilt.append((key, value))
    if not inserted:
        rebuilt.append((sensor_key, sensor_touch))
        rebuilt.append((valve_key, valve_touch))
    window5.clear()
    for key, value in rebuilt:
        window5[key] = value

    window10.update({
        "id": 110017,
        "visible": False,
        "touchable": True,
        "beepEnable": True,
        "backgroundPic": "window5_test_address_tip_opaque_560x220.png",
        "position": {"left": 235, "top": 81, "width": 560, "height": 220},
    })
    window10[next_key(data, window10, "textview")] = {
        "caption": "Window5TypePopupTitleText",
        "id": 50095,
        "text": "选择类型",
        "visible": False,
        "touchable": False,
        "family": FONT_FAMILY,
        "fontSize": 26,
        "bold": True,
        "alignment": 37,
        "colorTab": {"color0": 23483},
        "position": {"left": 40, "top": 16, "width": 480, "height": 36},
    }

    positions = {
        "Window5TypeRainButton": {"left": 72, "top": 44, "width": 178, "height": 80},
        "Window5TypeHumidityButton": {"left": 310, "top": 44, "width": 178, "height": 80},
        "Window5TypePressureButton": {"left": 72, "top": 124, "width": 178, "height": 80},
        "Window5TypeFlowButton": {"left": 310, "top": 124, "width": 178, "height": 80},
        "Window5TypeACValveButton": {"left": 40, "top": 70, "width": 240, "height": 120},
        "Window5TypeDCValveButton": {"left": 280, "top": 70, "width": 240, "height": 120},
    }
    for caption, position in positions.items():
        add_button(data, window10, caption, position)

    path.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(path)
    if decoded != data:
        raise RuntimeError("main.ftu encode/decode round-trip mismatch")


def main() -> int:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    shutil.copy2(UI_DIR / "main.ftu", BACKUP_DIR / "ui" / "main.ftu")
    for _caption, (_control_id, pic, label, source_icon) in BUTTONS.items():
        make_button(RESOURCE_DIR / pic, label, source_icon)
        sync_resource(pic)
    apply_ftu()
    print(f"backup={BACKUP_DIR}")
    print("Window10 type popup controls added")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
