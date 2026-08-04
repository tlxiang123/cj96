#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys
import time

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, RESOURCE_DIR, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
BACKUP_DIR = ROOT / "Release" / f"before_window5_white_regions_{time.strftime('%Y%m%d_%H%M%S')}"

BLUE = (0, 128, 255, 255)
WHITE = (255, 255, 255, 255)
LEFT_REGION = "debug_region_left_430x340.png"
RIGHT_REGION = "debug_region_right_405x340.png"
INPUT_VISUAL = "debug_edit_inner_99x71.png"
SMALL_OUTLINE = "debug_outline_82x58.png"


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


def make_region_png(path: Path, size: tuple[int, int]) -> None:
    scale = 4
    width, height = size
    image = Image.new("RGBA", (width * scale, height * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    radius = 11 * scale
    outline_width = 2 * scale
    # Keep the border inside the image and use downsampling for anti-aliasing.
    box = (
        outline_width // 2,
        outline_width // 2,
        width * scale - outline_width // 2 - 1,
        height * scale - outline_width // 2 - 1,
    )
    draw.rounded_rectangle(
        box,
        radius=radius,
        fill=WHITE,
        outline=BLUE,
        width=outline_width,
    )
    image = image.resize((width, height), Image.Resampling.LANCZOS)
    image.save(path)


def make_input_visual() -> None:
    source = RESOURCE_DIR / SMALL_OUTLINE
    target = RESOURCE_DIR / INPUT_VISUAL
    if not source.is_file():
        raise FileNotFoundError(source)
    canvas = Image.new("RGBA", (99, 71), (0, 0, 0, 0))
    with Image.open(source) as image:
        small = image.convert("RGBA")
    canvas.paste(small, ((99 - small.width) // 2, (71 - small.height) // 2), small)
    canvas.save(target)


def backup() -> None:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    (BACKUP_DIR / "resources").mkdir(parents=True, exist_ok=True)
    shutil.copy2(UI_DIR / "main.ftu", BACKUP_DIR / "ui" / "main.ftu")
    for name in (LEFT_REGION, RIGHT_REGION, INPUT_VISUAL):
        source = RESOURCE_DIR / name
        if source.is_file():
            shutil.copy2(source, BACKUP_DIR / "resources" / name)


def apply_ftu() -> None:
    path = UI_DIR / "main.ftu"
    data, header, _ = decode_ftu(path)
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found")

    left = find_caption(window5, "Window5LeftRegion")
    right = find_caption(window5, "Window5RightRegion")
    if left is None or right is None:
        raise RuntimeError("Window5 region not found")
    left["position"] = {"left": 30, "top": 27, "width": 430, "height": 340}
    left["backgroundPic"] = LEFT_REGION
    left["touchable"] = False
    right["position"] = {"left": 575, "top": 27, "width": 405, "height": 340}
    right["backgroundPic"] = RIGHT_REGION
    right["touchable"] = False

    for caption in ("ValveAddressEditText", "SrouceAddressEditText", "DestAdressEditText"):
        node = find_caption(window5, caption)
        if node is None:
            raise RuntimeError(f"{caption} not found")
        node["backgroundPic"] = INPUT_VISUAL
        node["alignment"] = 37
        node["fontSize"] = 30
        node["colorTab"] = {"color0": 23483, "color1": 23483, "color2": 23483}
        node.pop("textAlign", None)
        node.pop("align", None)
        node.pop("gravity", None)
        pos = node.get("position") or {}
        node["position"] = {
            "left": int(pos.get("left", 0)),
            "top": int(pos.get("top", 0)),
            "width": 99,
            "height": 71,
        }

    path.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(path)
    if decoded != data:
        raise RuntimeError("round-trip mismatch")


def verify() -> None:
    data, _, _ = decode_ftu(UI_DIR / "main.ftu")
    window5 = find_caption(data, "Window5")
    left = find_caption(window5, "Window5LeftRegion")
    right = find_caption(window5, "Window5RightRegion")
    if left.get("position", {}).get("height") != right.get("position", {}).get("height"):
        raise RuntimeError("region heights differ")
    if left.get("position", {}).get("top") != right.get("position", {}).get("top"):
        raise RuntimeError("region tops differ")
    for caption in ("ValveAddressEditText", "SrouceAddressEditText", "DestAdressEditText"):
        node = find_caption(window5, caption)
        if node.get("alignment") != 37:
            raise RuntimeError(f"{caption} alignment is not 37")
        if node.get("backgroundPic") != INPUT_VISUAL:
            raise RuntimeError(f"{caption} backgroundPic mismatch")


def main() -> None:
    backup()
    make_region_png(RESOURCE_DIR / LEFT_REGION, (430, 340))
    make_region_png(RESOURCE_DIR / RIGHT_REGION, (405, 340))
    make_input_visual()
    apply_ftu()
    verify()
    print(f"backup={BACKUP_DIR}")
    print("Window5 regions are white anti-aliased with equal height; address inputs centered")


if __name__ == "__main__":
    main()
