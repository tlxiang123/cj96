#!/usr/bin/env python3
"""Restyle the working demo Wi-Fi UI without changing its control tree."""

from __future__ import annotations

import copy
import sys
from pathlib import Path

from PIL import Image

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
UI_PATH = ROOT / "ui" / "wifisetting.ftu"
STYLE_SOURCE = ROOT / "Release" / "wifi_before_demo_20260720_095136" / "ui" / "wifisetting.ftu"
FRAME_PATH = ROOT / "resources" / "topset_inner_frame_967x360.png"
BACKGROUND_NAME = "wifi_page_rounded_1024x600.png"
BACKGROUND_PATH = ROOT / "resources" / BACKGROUND_NAME
TOGGLE_OFF_NAME = "window3_cycle_toggle_off_65.png"
TOGGLE_ON_NAME = "window3_cycle_toggle_on_65.png"
TOGGLE_INVALID_NAME = "window3_cycle_toggle_on_invalid_65.png"
HEADER_ICON_SOURCE_NAME = "topset_wifi_113.png"
HEADER_ICON_NAME = "topset_wifi_header_82.png"
HEADER_ICON_SIZE = 82


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


def copy_visual(target: dict, source: dict, fields: tuple[str, ...]) -> None:
    for field in fields:
        if field in source:
            target[field] = copy.deepcopy(source[field])


def build_background() -> None:
    page = Image.new("RGB", (1024, 600), "white")
    frame = Image.open(FRAME_PATH).convert("RGBA")
    page.paste(frame, (28, 113), frame)
    page.save(BACKGROUND_PATH, optimize=True)


def build_header_icon() -> None:
    source = Image.open(RESOURCE_DIR / HEADER_ICON_SOURCE_NAME).convert("RGBA")
    source.resize(
        (HEADER_ICON_SIZE, HEADER_ICON_SIZE), Image.Resampling.LANCZOS
    ).save(RESOURCE_DIR / HEADER_ICON_NAME, optimize=True)


def style_header_icon(data: dict) -> None:
    title = find_caption(data, "Textview1")
    title["backgroundPic"] = HEADER_ICON_NAME
    title["text"] = ""
    title["position"] = {
        "left": 471,
        "top": 9,
        "width": HEADER_ICON_SIZE,
        "height": HEADER_ICON_SIZE,
    }
    title.pop("textPosition", None)


def main() -> None:
    data, header, _ = decode_ftu(UI_PATH)
    style, _, _ = decode_ftu(STYLE_SOURCE)

    root = find_caption(data, "Window1")
    root["backgroundColor"] = 0xFFFFFF
    root["backgroundPic"] = BACKGROUND_NAME

    listview = find_caption(data, "ListViewWifiInfo")
    styled_list = find_caption(style, "ListViewWifiInfo")
    listview["position"] = {"left": 58, "top": 133, "width": 907, "height": 320}
    listview["backgroundColor"] = -1
    listview["visible"] = True
    listview["item"] = copy.deepcopy(styled_list["item"])
    listview["item"]["backgroundColor"] = -1
    listview["item"].setdefault("bgColorTab", {})["color0"] = -1
    for subitem in listview["item"].get("subItem", []):
        # Preserve the one-pixel divider, but let all content rows show the
        # rounded panel's real background instead of painting opaque white.
        if subitem.get("caption") != "SubItem1":
            subitem["backgroundColor"] = -1

    header_window = find_caption(data, "Window2")
    header_window["position"] = {"left": 0, "top": 0, "width": 1024, "height": 100}
    header_window["backgroundColor"] = 0xFFFFFF

    visual_fields = (
        "alignment",
        "bold",
        "colorTab",
        "family",
        "fontSize",
        "iconPosition",
        "picTab",
        "position",
        "textPosition",
    )
    for caption in ("sys_back", "Textview1", "ButtonOnOff"):
        copy_visual(find_caption(data, caption), find_caption(style, caption), visual_fields)

    style_header_icon(data)

    # Keep the switch artwork from the current window3 design.  The backup UI
    # uses the older 65px toggle assets, which would otherwise be copied back
    # every time this style script is re-run after an IDE auto-save.
    toggle = find_caption(data, "ButtonOnOff")
    toggle["picTab"] = {
        "pic0": TOGGLE_OFF_NAME,
        "pic2": TOGGLE_ON_NAME,
        "pic4": TOGGLE_INVALID_NAME,
    }

    # Keep the demo's network-info button and move it away from the wider toggle.
    menu = find_caption(data, "ButtonMenu")
    menu["position"] = {"left": 830, "top": 30, "width": 40, "height": 40}

    divider = find_caption(data, "Textview8")
    divider["position"] = {"left": 0, "top": 99, "width": 1024, "height": 1}

    build_background()
    build_header_icon()
    UI_PATH.write_bytes(encode_ftu(data, header))

    # Fail immediately if a style edit accidentally changes runtime bindings.
    expected = {
        "ButtonOnOff": 20005,
        "ButtonMenu": 20002,
        "sys_back": 100,
        "ListViewWifiInfo": 80000,
        "LISTSUBITEM_LEVEL": 70000,
        "LISTSUBITEM_NAME": 70001,
        "LISTSUBITEM_SUB": 70002,
    }
    verified, _, _ = decode_ftu(UI_PATH)
    for caption, control_id in expected.items():
        actual = find_caption(verified, caption).get("id")
        if actual != control_id:
            raise RuntimeError(f"{caption}: expected ID {control_id}, got {actual}")

    print(f"styled {UI_PATH}")
    print(f"generated {BACKGROUND_PATH}")


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    main()
