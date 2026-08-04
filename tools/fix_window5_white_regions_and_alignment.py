#!/usr/bin/env python3
from __future__ import annotations

import shutil
import sys
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
MAIN_FTU = ROOT / "ui" / "main.ftu"

BLUE = (0, 126, 255, 255)
TEXT_BLUE = (0, 92, 190, 255)
WHITE = (255, 255, 255, 255)

BUTTON_IMAGES = [
    "Open_Valve.png",
    "Close_Valve.png",
    "Sensor.png",
    "Sensor_Selected.png",
    "Solenoid_Valve.png",
    "Solenoid_Valve_Selected.png",
    "Source_Address.png",
    "Target_Address.png",
    "Edit_Address.png",
    "Test_Address.png",
    "Force_Edit.png",
]


def font(size: int) -> ImageFont.FreeTypeFont:
    candidates = [
        ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf",
        RESOURCES / "font" / "Alibaba-PuHuiTi-Regular.ttf",
        Path(r"C:\Windows\Fonts\msyh.ttc"),
        Path(r"C:\Windows\Fonts\simhei.ttf"),
    ]
    for path in candidates:
        if path.exists():
            return ImageFont.truetype(str(path), size)
    return ImageFont.load_default()


def backup_files() -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup = ROOT / "Release" / f"before_window5_white_region_align_{stamp}"
    backup.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup / "main.ftu")
    for name in BUTTON_IMAGES + [
        "window5_region1_frame_430x340.png",
        "window5_region2_frame_405x340.png",
    ]:
        src = RESOURCES / name
        if src.exists():
            shutil.copy2(src, backup / name)
    return backup


def whiten_asset_edges(path: Path) -> None:
    image = Image.open(path).convert("RGBA")
    pixels = image.load()
    width, height = image.size
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            # The reference screenshots carried slightly different near-white
            # backgrounds.  In Window5 the regions are white, so normalize every
            # near-white / transparent edge pixel to exact white.
            if a < 255 or (r > 218 and g > 225 and b > 230):
                pixels[x, y] = WHITE
    image.save(path, optimize=True)


def blue_mask_icon(source: Image.Image, box: tuple[int, int, int, int], target_size: tuple[int, int]) -> Image.Image:
    crop = source.crop(box).convert("RGBA")
    # Keep only strong blue strokes; turn near-white screenshot background transparent.
    pixels = crop.load()
    width, height = crop.size
    for y in range(height):
        for x in range(width):
            r, g, b, a = pixels[x, y]
            is_blue = b > 120 and g > 55 and r < 120
            pixels[x, y] = (0, 105, 210, a if is_blue else 0)
    crop = crop.resize(target_size, Image.Resampling.LANCZOS)
    return crop


def draw_open_valve_like_close() -> None:
    current = Image.open(RESOURCES / "Open_Valve.png").convert("RGBA")
    icon = blue_mask_icon(current, (0, 0, 68, 58), (48, 42))

    image = Image.new("RGBA", (120, 60), WHITE)
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((1, 1, 118, 58), radius=7, fill=WHITE, outline=BLUE, width=2)
    image.alpha_composite(icon, (12, 9))

    text_font = font(18)
    text = "开阀"
    bbox = draw.textbbox((0, 0), text, font=text_font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]
    draw.text((75 - text_w // 2 + 18, 30 - text_h // 2 - 1), text, font=text_font, fill=TEXT_BLUE)

    image.save(RESOURCES / "Open_Valve.png", optimize=True)


def draw_region_frame(path: Path, size: tuple[int, int]) -> None:
    width, height = size
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((1, 1, width - 2, height - 2), radius=8, fill=WHITE, outline=BLUE, width=2)
    image.save(path, optimize=True)


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, (list, tuple)):
        for value in node:
            yield from walk(value)


def apply_layout() -> None:
    data, header, _offset = decode_ftu(MAIN_FTU)
    positions = {
        # Region 2 top buttons were overlapping the address edit boxes.
        "TextView12": {"left": 612, "top": 40, "width": 120, "height": 60},
        "TextView17": {"left": 850, "top": 40, "width": 120, "height": 60},
        "SrouceAddressEditText": {"left": 613, "top": 118, "width": 99, "height": 71},
        "DestAdressEditText": {"left": 840, "top": 118, "width": 99, "height": 71},
    }
    seen: set[str] = set()
    for node in walk(data):
        caption = node.get("caption")
        if caption in positions:
            node["position"] = dict(positions[caption])
            seen.add(caption)
        if caption in {"Window5Region1Frame", "Window5Region2Frame"}:
            node["text"] = ""
            node["bgColorTab"] = {"color0": 0xFFFFFF}
    missing = set(positions) - seen
    if missing:
        raise RuntimeError(f"missing controls: {sorted(missing)}")
    MAIN_FTU.write_bytes(encode_ftu(data, header))


def main() -> None:
    backup = backup_files()

    draw_region_frame(RESOURCES / "window5_region1_frame_430x340.png", (430, 340))
    draw_region_frame(RESOURCES / "window5_region2_frame_405x340.png", (405, 340))

    for name in BUTTON_IMAGES:
        path = RESOURCES / name
        if path.exists():
            whiten_asset_edges(path)
    draw_open_valve_like_close()
    whiten_asset_edges(RESOURCES / "Open_Valve.png")

    apply_layout()

    print(f"fixed Window5 white regions, button edges, open-valve layout, and region2 overlap; backup={backup}")


if __name__ == "__main__":
    main()
