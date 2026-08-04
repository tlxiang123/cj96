#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import shutil
import sys
import time

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
UI_DIR = ROOT / "ui"
RESOURCE_DIR = ROOT / "resources"
RELEASE = ROOT / "Release"

BACKGROUND = "rain_delay_dialog_520x250.png"
TEXT_BLUE = 23483
TEXT_DARK = 0

CONTROL_IDS = {
    "RainDelayWindow": 110090,
    "RainDelayTouchButton": 20210,
    "RainDelayIcon": 20211,
    "RainDelayCancelButton": 20212,
    "RainDelayOkButton": 20213,
    "RainDelayTitleText": 50290,
    "RainDelayHintText": 50291,
    "RainDelayUnitText": 50292,
    "RainDelayDaysEditText": 51045,
}


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def find_caption(node: object, caption: str) -> dict:
    for item in walk(node):
        if isinstance(item, dict) and item.get("caption") == caption:
            return item
    raise LookupError(caption)


def collect_ids(node: object) -> dict[int, str]:
    values: dict[int, str] = {}
    for item in walk(node):
        if isinstance(item, dict) and isinstance(item.get("id"), int):
            values[item["id"]] = str(item.get("caption", ""))
    return values


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
    suffix = max_numeric_suffix(root, prefix) + 1
    while f"{prefix}__{suffix}" in parent:
        suffix += 1
    return f"{prefix}__{suffix}"


def remove_caption(parent: dict, caption: str) -> None:
    for key in list(parent.keys()):
        value = parent[key]
        if isinstance(value, dict) and value.get("caption") == caption:
            del parent[key]


def text_node(caption: str, control_id: int, text: str, left: int, top: int,
              width: int, height: int, font_size: int, color: int = TEXT_BLUE,
              bold: bool = False, alignment: int = 37) -> dict:
    node = {
        "alignment": alignment,
        "caption": caption,
        "colorTab": {"color0": color},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": font_size,
        "id": control_id,
        "text": text,
        "touchable": False,
        "position": {"height": height, "left": left, "top": top, "width": width},
    }
    if bold:
        node["bold"] = True
    return node


def button_node(caption: str, control_id: int, left: int, top: int, width: int,
                height: int, text: str = "", font_size: int = 24,
                pic: str | None = None) -> dict:
    node = {
        "alignment": 37,
        "beepEnable": True,
        "caption": caption,
        "colorTab": {"color0": TEXT_BLUE, "color1": TEXT_BLUE, "color2": TEXT_BLUE},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": font_size,
        "id": control_id,
        "position": {"height": height, "left": left, "top": top, "width": width},
    }
    if text:
        node["text"] = text
    if pic:
        node["picTab"] = {"pic0": pic, "pic1": pic, "pic2": pic}
        node["iconPosition"] = {"height": height, "left": 0, "top": 0, "width": width}
    return node


def edit_node(caption: str, control_id: int, text: str, left: int, top: int,
              width: int, height: int) -> dict:
    return {
        "alignment": 37,
        "beepEnable": True,
        "bgColorTab": {"color0": 13624314},
        "caption": caption,
        "colorTab": {"color0": TEXT_DARK},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 24,
        "hintTextColor": TEXT_BLUE,
        "id": control_id,
        "text": text,
        "textType": 1,
        "position": {"height": height, "left": left, "top": top, "width": width},
    }


def create_background() -> None:
    img = Image.new("RGBA", (520, 250), (248, 252, 255, 255))
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle(
        (1, 1, 518, 248),
        radius=12,
        fill=(249, 253, 255, 255),
        outline=(0, 132, 255, 255),
        width=3,
    )
    for path in (UI_DIR / BACKGROUND, RESOURCE_DIR / BACKGROUND):
        path.parent.mkdir(parents=True, exist_ok=True)
        img.save(path)


def main() -> int:
    create_background()
    data, header, _ = decode_ftu(MAIN_FTU)

    ids = collect_ids(data)
    for caption, control_id in CONTROL_IDS.items():
        existing = ids.get(control_id)
        if existing and existing != caption:
            raise RuntimeError(f"id {control_id} collides with {existing}")

    stamp = time.strftime("%Y%m%d_%H%M%S")
    backup_dir = RELEASE / f"rain_delay_popup_backup_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup_dir / "main.before.ftu")

    window1 = find_caption(data, "Window1")
    window7 = find_caption(window1, "Window7")

    remove_caption(window1, "RainDelayWindow")
    remove_caption(window7, "RainDelayTouchButton")

    rain_icon = find_caption(window7, "Button48")
    rain_text = find_caption(window7, "TextView19")
    rain_icon["touchable"] = True
    rain_icon["beepEnable"] = True
    rain_text["touchable"] = True

    touch = {
        "caption": "RainDelayTouchButton",
        "id": 20210,
        "position": {"height": 64, "left": 120, "top": 8, "width": 220},
    }
    window7[next_key(data, window7, "button")] = touch

    dialog = {
        "backgroundPic": BACKGROUND,
        "beepEnable": True,
        "caption": "RainDelayWindow",
        "id": 110090,
        "visible": False,
        "position": {"height": 250, "left": 245, "top": 72, "width": 520},
    }
    dialog[next_key(data, dialog, "textview")] = text_node(
        "RainDelayTitleText", 50290, "雨雪延后设置", 0, 14, 520, 36, 26, TEXT_BLUE, True
    )
    delay_icon_key = next_key(data, dialog, "button")
    dialog[delay_icon_key] = button_node(
        "RainDelayIcon", 20211, 88, 76, 96, 96, pic="window3_delay_96.png"
    )
    dialog[delay_icon_key]["beepEnable"] = False
    dialog[delay_icon_key]["touchable"] = False
    dialog[next_key(data, dialog, "textview")] = text_node(
        "RainDelayHintText", 50291, "检测到雨雪后，当天计划不执行", 205, 70, 245, 34, 20, TEXT_BLUE
    )
    dialog[next_key(data, dialog, "edittext")] = edit_node(
        "RainDelayDaysEditText", 51045, "1", 245, 115, 80, 39
    )
    dialog[next_key(data, dialog, "textview")] = text_node(
        "RainDelayUnitText", 50292, "天", 340, 119, 48, 34, 24, TEXT_DARK
    )
    dialog[next_key(data, dialog, "button")] = button_node(
        "RainDelayCancelButton", 20212, 125, 180, 120, 60, pic="set_runtime_cancel_same_font_120x60.png"
    )
    dialog[next_key(data, dialog, "button")] = button_node(
        "RainDelayOkButton", 20213, 275, 180, 120, 60, pic="set_runtime_ok_same_font_120x60.png"
    )
    window1[next_key(data, window1, "window")] = dialog

    MAIN_FTU.write_bytes(encode_ftu(data, header))
    shutil.copy2(MAIN_FTU, backup_dir / "main.after.ftu")
    print(f"updated {MAIN_FTU}")
    print(f"backup {backup_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
