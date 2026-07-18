#!/usr/bin/env python3
"""Add an inset Window1-style frame without changing current page parameters."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "page1topset.ftu"
RESOURCE_DIR = ROOT / "resources"
SCALE = 4
BUTTON_CAPTIONS = (
    "OpenWifiButton",
    "OpenNetButton",
    "Open4GButton",
    "SetSysTimeBtn",
    "LanBtn",
    "DisplayBtn",
)


def find_control(node: object, caption: str) -> tuple[dict, str, dict] | None:
    if not isinstance(node, dict):
        return None
    for key, value in node.items():
        if not isinstance(value, dict):
            continue
        if value.get("caption") == caption:
            return node, key, value
        found = find_control(value, caption)
        if found is not None:
            return found
    return None


def build_window1_panel(width: int, height: int) -> Image.Image:
    large_size = (width * SCALE, height * SCALE)
    panel = Image.new("RGBA", large_size, (0, 0, 0, 0))
    gradient = Image.new("RGBA", large_size)
    draw = ImageDraw.Draw(gradient)
    top = (254, 254, 255, 255)
    bottom = (232, 244, 255, 255)
    for y in range(large_size[1]):
        ratio = y / max(1, large_size[1] - 1)
        color = tuple(
            round(top[channel] + (bottom[channel] - top[channel]) * ratio)
            for channel in range(4)
        )
        draw.line((0, y, large_size[0] - 1, y), fill=color)

    inset = 2 * SCALE
    box = (inset, inset, large_size[0] - inset - 1, large_size[1] - inset - 1)
    mask = Image.new("L", large_size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(box, radius=18 * SCALE, fill=255)
    panel.paste(gradient, (0, 0), mask)
    ImageDraw.Draw(panel).rounded_rectangle(
        box,
        radius=18 * SCALE,
        outline=(60, 160, 234, 255),
        width=2 * SCALE,
    )
    return panel.resize((width, height), Image.Resampling.LANCZOS)


def main() -> None:
    data, header, _ = decode_ftu(FTU_PATH)
    content_found = find_control(data, "TopSetContentWindow")
    page_found = find_control(data, "TopSetPageWindow")
    if content_found is None or page_found is None:
        raise RuntimeError("current TopSet windows are missing")

    content = content_found[2]
    page = page_found[2]
    preserved_content_position = dict(content["position"])
    preserved_content_color = content.get("backgroundColor")
    preserved_page = {
        key: value for key, value in page.items() if not isinstance(value, dict)
    }

    inset = 20
    frame_width = content["position"]["width"] - inset * 2
    frame_height = content["position"]["height"] - inset * 2
    if frame_width <= 0 or frame_height <= 0:
        raise RuntimeError("TopSetContentWindow is too small for a 20px inset")

    old_frame = find_control(content, "TopSetContentFrameWindow")
    if old_frame is not None:
        old_parent, old_key, old_node = old_frame
        old_left = old_node["position"]["left"]
        old_top = old_node["position"]["top"]
        for caption in BUTTON_CAPTIONS:
            button_found = find_control(old_node, caption)
            if button_found is not None:
                button = button_found[2]
                button["position"]["left"] += old_left
                button["position"]["top"] += old_top
        del old_parent[old_key]

    buttons: list[dict] = []
    for caption in BUTTON_CAPTIONS:
        found = find_control(content, caption)
        if found is None:
            raise RuntimeError(f"missing current button: {caption}")
        parent, key, button = found
        del parent[key]
        button["position"]["left"] -= inset
        button["position"]["top"] -= inset
        buttons.append(button)

    frame_name = f"topset_inner_frame_{frame_width}x{frame_height}.png"
    build_window1_panel(frame_width, frame_height).save(
        RESOURCE_DIR / frame_name, optimize=True
    )
    frame = {
        "backgroundPic": frame_name,
        "beepEnable": True,
        "caption": "TopSetContentFrameWindow",
        "id": 110004,
        "position": {
            "height": frame_height,
            "left": inset,
            "top": inset,
            "width": frame_width,
        },
    }
    for index, button in enumerate(buttons, start=1):
        frame[f"button__{index}"] = button
    content["window__20"] = frame

    FTU_PATH.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(FTU_PATH)
    verified_content = find_control(decoded, "TopSetContentWindow")[2]
    verified_page = find_control(decoded, "TopSetPageWindow")[2]
    verified_page_scalars = {
        key: value for key, value in verified_page.items() if not isinstance(value, dict)
    }
    if verified_content["position"] != preserved_content_position:
        raise RuntimeError("TopSetContentWindow position was changed")
    if verified_content.get("backgroundColor") != preserved_content_color:
        raise RuntimeError("TopSetContentWindow color was changed")
    if verified_page_scalars != preserved_page:
        raise RuntimeError("TopSetPageWindow parameters were changed")
    print(
        f"preserved current colors/positions; added {frame_width}x{frame_height} "
        "Window1-style frame at 20,20"
    )


if __name__ == "__main__":
    main()
