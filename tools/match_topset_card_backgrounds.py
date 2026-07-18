#!/usr/bin/env python3
"""Bake the panel pixels behind each TopSet card and align button colors."""

from __future__ import annotations

from pathlib import Path

from PIL import Image

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "page1topset.ftu"
RESOURCE_DIR = ROOT / "resources"
BUTTON_ASSETS = {
    "OpenWifiButton": "topset_wifi_113.png",
    "OpenNetButton": "topset_ethernet_113.png",
    "Open4GButton": "topset_4g_113.png",
    "SetSysTimeBtn": "topset_time_113.png",
    "LanBtn": "topset_language_113.png",
    "DisplayBtn": "topset_display_113.png",
}


def find_control(node: object, caption: str) -> dict | None:
    if not isinstance(node, dict):
        return None
    if node.get("caption") == caption:
        return node
    for value in node.values():
        if isinstance(value, dict):
            found = find_control(value, caption)
            if found is not None:
                return found
    return None


def color_value(pixel: tuple[int, int, int, int]) -> int:
    red, green, blue, _ = pixel
    return (red << 16) | (green << 8) | blue


def main() -> None:
    data, header, _ = decode_ftu(FTU_PATH)
    frame = find_control(data, "TopSetContentFrameWindow")
    content = find_control(data, "TopSetContentWindow")
    if frame is None or content is None:
        raise RuntimeError("current TopSet frame/content window is missing")

    preserved_content_color = content.get("backgroundColor")
    preserved_content_position = dict(content["position"])
    panel = Image.open(RESOURCE_DIR / frame["backgroundPic"]).convert("RGBA")

    for caption, asset_name in BUTTON_ASSETS.items():
        button = find_control(frame, caption)
        if button is None:
            raise RuntimeError(f"missing framed button: {caption}")
        position = button["position"]
        left = position["left"]
        top = position["top"]
        width = position["width"]
        height = position["height"]
        background = panel.crop((left, top, left + width, top + height))
        card = Image.open(RESOURCE_DIR / asset_name).convert("RGBA")
        if card.size != background.size:
            raise RuntimeError(f"wrong card size: {asset_name}: {card.size}")
        baked = Image.alpha_composite(background, card).convert("RGB")
        baked.save(RESOURCE_DIR / asset_name, optimize=True)

        center = panel.getpixel((left + width // 2, top + height // 2))
        matched_color = color_value(center)
        button["bgColorTab"] = {
            "color0": matched_color,
            "color1": matched_color,
        }

    FTU_PATH.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(FTU_PATH)
    verified_content = find_control(decoded, "TopSetContentWindow")
    if verified_content.get("backgroundColor") != preserved_content_color:
        raise RuntimeError("TopSetContentWindow color was changed")
    if verified_content["position"] != preserved_content_position:
        raise RuntimeError("TopSetContentWindow position was changed")

    for caption, asset_name in BUTTON_ASSETS.items():
        button = find_control(decoded, caption)
        image = Image.open(RESOURCE_DIR / asset_name).convert("RGBA")
        if image.getchannel("A").getextrema() != (255, 255):
            raise RuntimeError(f"card is not fully opaque: {asset_name}")
        position = button["position"]
        panel_corner = panel.getpixel((position["left"], position["top"]))[:3]
        if image.getpixel((0, 0))[:3] != panel_corner:
            raise RuntimeError(f"corner background mismatch: {asset_name}")
    print("baked exact panel backgrounds into six opaque TopSet cards")


if __name__ == "__main__":
    main()
