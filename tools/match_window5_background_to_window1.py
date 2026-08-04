#!/usr/bin/env python3
from __future__ import annotations

import shutil
import sys
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
MAIN_FTU = ROOT / "ui" / "main.ftu"

WINDOW5_BG = "window5_background_1007x400.png"
REGION1 = "window5_region1_frame_430x340.png"
REGION2 = "window5_region2_frame_405x340.png"

BASE_TOP = (213, 233, 252, 255)
BASE_BOTTOM = (198, 224, 247, 255)
PANEL_TOP = (253, 255, 255, 255)
PANEL_BOTTOM = (232, 246, 255, 255)
BLUE = (0, 126, 255, 255)


def backup_files() -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup = ROOT / "Release" / f"before_window5_window1_background_{stamp}"
    backup.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup / "main.ftu")
    for name in [WINDOW5_BG, REGION1, REGION2]:
        src = RESOURCES / name
        if src.exists():
            shutil.copy2(src, backup / name)
    return backup


def lerp(c0: tuple[int, int, int, int], c1: tuple[int, int, int, int], t: float) -> tuple[int, int, int, int]:
    return tuple(int(c0[i] * (1.0 - t) + c1[i] * t) for i in range(4))


def vertical_gradient(size: tuple[int, int], top: tuple[int, int, int, int], bottom: tuple[int, int, int, int]) -> Image.Image:
    width, height = size
    image = Image.new("RGBA", size)
    pixels = image.load()
    for y in range(height):
        color = lerp(top, bottom, y / max(1, height - 1))
        for x in range(width):
            pixels[x, y] = color
    return image


def draw_window5_background() -> None:
    image = vertical_gradient((1007, 400), BASE_TOP, BASE_BOTTOM)
    image.save(RESOURCES / WINDOW5_BG, optimize=True)


def draw_region(path: Path, size: tuple[int, int]) -> None:
    width, height = size
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    mask = Image.new("L", size, 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.rounded_rectangle((1, 1, width - 2, height - 2), radius=8, fill=255)
    panel = vertical_gradient(size, PANEL_TOP, PANEL_BOTTOM)
    image.alpha_composite(Image.composite(panel, Image.new("RGBA", size, (0, 0, 0, 0)), mask))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((1, 1, width - 2, height - 2), radius=8, outline=BLUE, width=2)
    path.write_bytes(b"")
    image.save(path, optimize=True)


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def update_ftu() -> None:
    data, header, _ = decode_ftu(MAIN_FTU)
    seen: set[str] = set()
    for node in walk(data):
        caption = node.get("caption")
        if caption == "Window5":
            node["backgroundPic"] = WINDOW5_BG
            node.pop("bgColorTab", None)
            seen.add(caption)
        elif caption == "Window5Region1Frame":
            node["backgroundPic"] = REGION1
            node["text"] = ""
            node["bgColorTab"] = {"color0": 0xCFE3FA}
            seen.add(caption)
        elif caption == "Window5Region2Frame":
            node["backgroundPic"] = REGION2
            node["text"] = ""
            node["bgColorTab"] = {"color0": 0xCFE3FA}
            seen.add(caption)
    missing = {"Window5", "Window5Region1Frame", "Window5Region2Frame"} - seen
    if missing:
        raise RuntimeError(f"missing controls: {sorted(missing)}")
    MAIN_FTU.write_bytes(encode_ftu(data, header))


def main() -> None:
    backup = backup_files()
    draw_window5_background()
    draw_region(RESOURCES / REGION1, (430, 340))
    draw_region(RESOURCES / REGION2, (405, 340))
    update_ftu()
    print(f"matched Window5 base and region gradients to Window1 style; backup={backup}")


if __name__ == "__main__":
    main()
