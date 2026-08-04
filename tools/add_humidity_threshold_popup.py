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

BACKGROUND = "humidity_threshold_dialog_520x250.png"
ICON = "water_icon_96_aa.png"
SOURCE_ICON = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-352ae44f-c3d5-467c-a5d8-e856c80f7ee2.png")
TEXT_BLUE = 23483
TEXT_DARK = 0

CONTROL_IDS = {
    "HumidityThresholdWindow": 110091,
    "HumidityThresholdTouchButton": 20214,
    "HumidityThresholdIcon": 20215,
    "HumidityThresholdCancelButton": 20216,
    "HumidityThresholdOkButton": 20217,
    "HumidityThresholdTitleText": 50293,
    "HumidityThresholdHintText": 50294,
    "HumidityThresholdUnitText": 50295,
    "HumidityThresholdEditText": 51046,
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
        "backgroundPic": "debug_edit_inner_99x71.png",
        "caption": caption,
        "colorTab": {"color0": TEXT_DARK},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 34,
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


def create_icon() -> None:
    src = SOURCE_ICON if SOURCE_ICON.is_file() else UI_DIR / "water_icon_48.png"
    source = Image.open(src).convert("RGBA")
    source.thumbnail((96, 96), Image.Resampling.LANCZOS)

    bg = (249, 253, 255)
    blue = (0, 132, 255)
    img = Image.new("RGBA", (96, 96), (*bg, 255))
    img.alpha_composite(source, ((96 - source.width) // 2, (96 - source.height) // 2))

    pixels = img.load()
    for y in range(img.height):
        for x in range(img.width):
            r, g, b, a = pixels[x, y]
            if a == 0 or (r >= 220 and g >= 220 and b >= 210):
                pixels[x, y] = (*bg, 255)
                continue
            blue_score = b - max(r, g)
            if blue_score > 10 or (b > 150 and g > 120 and r < 210):
                strength = min(255, max(48, int(((255 - r) * 0.45 + (255 - g) * 0.35 + blue_score * 0.7) * 2.35)))
                mixed = tuple(
                    int((channel * strength + bg_channel * (255 - strength)) / 255)
                    for channel, bg_channel in zip(blue, bg)
                )
                pixels[x, y] = (*mixed, 255)
    for path in (UI_DIR / ICON, RESOURCE_DIR / ICON):
        path.parent.mkdir(parents=True, exist_ok=True)
        img.save(path)


def main() -> int:
    create_background()
    create_icon()
    data, header, _ = decode_ftu(MAIN_FTU)

    ids = collect_ids(data)
    for caption, control_id in CONTROL_IDS.items():
        existing = ids.get(control_id)
        if existing and existing != caption:
            raise RuntimeError(f"id {control_id} collides with {existing}")

    stamp = time.strftime("%Y%m%d_%H%M%S")
    backup_dir = RELEASE / f"humidity_threshold_popup_backup_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup_dir / "main.before.ftu")

    window1 = find_caption(data, "Window1")
    window7 = find_caption(window1, "Window7")

    remove_caption(window1, "HumidityThresholdWindow")
    remove_caption(window7, "HumidityThresholdTouchButton")

    humidity_icon = find_caption(window7, "Button49")
    humidity_text = find_caption(window7, "TextView18")
    humidity_icon["touchable"] = True
    humidity_icon["beepEnable"] = True
    humidity_text["touchable"] = True

    window7[next_key(data, window7, "button")] = {
        "caption": "HumidityThresholdTouchButton",
        "id": CONTROL_IDS["HumidityThresholdTouchButton"],
        "position": {"height": 64, "left": 120, "top": 94, "width": 220},
    }

    dialog = {
        "backgroundPic": BACKGROUND,
        "beepEnable": True,
        "caption": "HumidityThresholdWindow",
        "id": CONTROL_IDS["HumidityThresholdWindow"],
        "visible": False,
        "position": {"height": 250, "left": 245, "top": 72, "width": 520},
    }
    dialog[next_key(data, dialog, "textview")] = text_node(
        "HumidityThresholdTitleText", CONTROL_IDS["HumidityThresholdTitleText"],
        "湿度触发设置", 0, 14, 520, 36, 26, TEXT_BLUE, True
    )
    icon_key = next_key(data, dialog, "button")
    dialog[icon_key] = button_node(
        "HumidityThresholdIcon", CONTROL_IDS["HumidityThresholdIcon"],
        88, 76, 96, 96, pic=ICON
    )
    dialog[icon_key]["beepEnable"] = False
    dialog[icon_key]["touchable"] = False
    dialog[next_key(data, dialog, "textview")] = text_node(
        "HumidityThresholdHintText", CONTROL_IDS["HumidityThresholdHintText"],
        "湿度达到阈值后触发", 205, 70, 245, 34, 20, TEXT_BLUE
    )
    dialog[next_key(data, dialog, "edittext")] = edit_node(
        "HumidityThresholdEditText", CONTROL_IDS["HumidityThresholdEditText"],
        "80", 245, 108, 99, 71
    )
    dialog[next_key(data, dialog, "textview")] = text_node(
        "HumidityThresholdUnitText", CONTROL_IDS["HumidityThresholdUnitText"],
        "%", 360, 126, 48, 34, 28, TEXT_DARK
    )
    dialog[next_key(data, dialog, "button")] = button_node(
        "HumidityThresholdCancelButton", CONTROL_IDS["HumidityThresholdCancelButton"],
        125, 180, 120, 60, pic="set_runtime_cancel_same_font_120x60.png"
    )
    dialog[next_key(data, dialog, "button")] = button_node(
        "HumidityThresholdOkButton", CONTROL_IDS["HumidityThresholdOkButton"],
        275, 180, 120, 60, pic="set_runtime_ok_same_font_120x60.png"
    )
    window1[next_key(data, window1, "window")] = dialog

    MAIN_FTU.write_bytes(encode_ftu(data, header))
    shutil.copy2(MAIN_FTU, backup_dir / "main.after.ftu")
    print(f"updated {MAIN_FTU}")
    print(f"backup {backup_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
