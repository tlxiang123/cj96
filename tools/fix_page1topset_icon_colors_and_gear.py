from __future__ import annotations

import shutil
from datetime import datetime
from pathlib import Path

import numpy as np
from PIL import Image

import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
FTU_PATH = ROOT / "ui" / "page1topset.ftu"

TARGET_COLOR = (2, 91, 187)
GEAR_NAME = "topset_settings_header_clean_158x108.png"
ICON_NAMES = (
    "topset_time_113.png",
    GEAR_NAME,
    "topset_wifi_113.png",
    "topset_ethernet_113.png",
    "topset_4g_113.png",
    "topset_display_113.png",
    "topset_language_113.png",
    "topset_debug_113.png",
    "topset_remote_upgrade_113.png",
)


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def recolor(path: Path) -> Image.Image:
    image = Image.open(path).convert("RGBA")
    pixels = np.asarray(image).copy()
    visible = pixels[:, :, 3] > 0
    pixels[visible, 0] = TARGET_COLOR[0]
    pixels[visible, 1] = TARGET_COLOR[1]
    pixels[visible, 2] = TARGET_COLOR[2]
    return Image.fromarray(pixels, "RGBA")


def rebuild_gear(image: Image.Image) -> Image.Image:
    alpha = image.getchannel("A")
    bounds = alpha.getbbox()
    if bounds is None:
        raise RuntimeError("gear image has no visible content")
    content = image.crop(bounds)
    canvas = Image.new("RGBA", (158, 108), (0, 0, 0, 0))
    canvas.alpha_composite(
        content,
        ((canvas.width - content.width) // 2, (canvas.height - content.height) // 2),
    )
    return canvas


def find_control(node: object, caption: str) -> dict | None:
    for value in walk(node):
        if isinstance(value, dict) and value.get("caption") == caption:
            return value
    return None


def main() -> None:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = ROOT / "backups" / f"page1topset_visual_fix_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=True)

    for name in ICON_NAMES:
        source = RESOURCE_DIR / name
        if not source.is_file():
            raise FileNotFoundError(source)
        shutil.copy2(source, backup_dir / name)
    shutil.copy2(FTU_PATH, backup_dir / FTU_PATH.name)

    for name in ICON_NAMES:
        image = recolor(RESOURCE_DIR / name)
        if name == GEAR_NAME:
            image = rebuild_gear(image)
        image.save(RESOURCE_DIR / name, optimize=True)

    data, header, _ = decode_ftu(FTU_PATH)
    language = find_control(data, "LanBtn")
    if language is None:
        raise RuntimeError("LanBtn was not found in page1topset.ftu")
    language.pop("bgColorTab", None)

    header_button = find_control(data, "Button1")
    if header_button is None:
        raise RuntimeError("Button1 was not found in page1topset.ftu")
    expected_position = {"height": 108, "left": 433, "top": 2, "width": 158}
    if header_button.get("position") != expected_position:
        raise RuntimeError(
            f"unexpected Button1 position: {header_button.get('position')!r}"
        )

    FTU_PATH.write_bytes(encode_ftu(data, header))
    verified, _, _ = decode_ftu(FTU_PATH)
    verified_language = find_control(verified, "LanBtn")
    if verified_language is None or "bgColorTab" in verified_language:
        raise RuntimeError("LanBtn background color was not removed")

    gear = Image.open(RESOURCE_DIR / GEAR_NAME).convert("RGBA")
    if gear.size != (158, 108):
        raise RuntimeError(f"gear canvas size is {gear.size}, expected 158x108")

    print(f"target_color=RGB{TARGET_COLOR}")
    print(f"backup={backup_dir}")
    print(f"updated_resources={RESOURCE_DIR}")
    print(f"updated_ftu={FTU_PATH}")
    print(f"gear_canvas={gear.size}")
    print("LanBtn.bgColorTab=removed")


if __name__ == "__main__":
    main()
