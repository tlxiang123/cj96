#!/usr/bin/env python3
"""Build and bind the Window10 device-name edit frame."""

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
UI_PATH = ROOT / "ui" / "main.ftu"
OUTPUT = RESOURCE_DIR / "w2_name_edit_204x61.png"


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    raise LookupError(caption)


def build_image() -> None:
    scale = 4
    width, height = 204, 61
    image = Image.new("RGB", (width * scale, height * scale), (247, 253, 255))
    pixels = image.load()
    for y in range(height * scale):
        ratio = y / max(1, height * scale - 1)
        color = (
            round(253 - 7 * ratio),
            round(255 - 2 * ratio),
            255,
        )
        for x in range(width * scale):
            pixels[x, y] = color

    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (2 * scale, 2 * scale, (width - 3) * scale, (height - 3) * scale),
        radius=8 * scale,
        outline=(18, 148, 232),
        width=2 * scale,
    )
    image.resize((width, height), Image.Resampling.LANCZOS).save(OUTPUT, optimize=True)


def bind_image() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    control = find_caption(layout, "W2_NameEditText")
    position = dict(control.get("position", {}))
    control["alignment"] = 37
    control["bold"] = True
    control["colorTab"] = {"color0": 23483}
    control["backgroundPic"] = OUTPUT.name
    UI_PATH.write_bytes(encode_ftu(layout, header))

    verified, _, _ = decode_ftu(UI_PATH)
    result = find_caption(verified, "W2_NameEditText")
    if result.get("position") != position or result.get("backgroundPic") != OUTPUT.name:
        raise RuntimeError("W2_NameEditText frame verification failed")
    print(f"bound {OUTPUT.name}; preserved position {position}")


def main() -> None:
    build_image()
    bind_image()


if __name__ == "__main__":
    main()
