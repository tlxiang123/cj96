#!/usr/bin/env python3
"""Set Button45 to the existing 120x60 clear-device artwork."""

from __future__ import annotations

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu
from set_runtime_buttons_match_font import BLUE, FONT, SCALE, blue_component_icon


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
PICTURE = "button45_delete_120x60.png"


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


def main() -> None:
    from PIL import Image, ImageDraw

    source = Image.open(ROOT / "ui" / "set_runtime_clear_same_font_120x60.png").convert("RGBA")
    trash_icon = blue_component_icon(source, (8, 9, 49, 51), (34 * SCALE, 38 * SCALE))
    template = Image.open(ROOT / "ui" / "w2_group_bind_cancel_120x60.png").convert("RGBA")

    # Preserve the exact panel-colored corners, border and gradient from the
    # neighboring button while removing only its interior icon and label.
    pixels = template.load()
    for y in range(6, 55):
        clean = template.getpixel((52, y))
        for x in range(6, 114):
            red, green, blue, alpha = pixels[x, y]
            if alpha and blue - green > 12 and blue - red > 20:
                pixels[x, y] = clean

    artwork = template
    trash_icon.thumbnail((34, 38), Image.Resampling.LANCZOS)
    artwork.alpha_composite(trash_icon, (15, (60 - trash_icon.height) // 2))
    draw = ImageDraw.Draw(artwork)
    native_font = FONT.font_variant(size=21)
    text_box = draw.textbbox((0, 0), "删除", font=native_font)
    text_height = text_box[3] - text_box[1]
    draw.text((55, (60 - text_height) // 2 - 1), "删除", font=native_font, fill=BLUE)
    for directory in (ROOT / "ui", ROOT / "resources"):
        artwork.save(directory / PICTURE, format="PNG", optimize=True)

    for directory in (ROOT / "ui", ROOT / "resources"):
        if not (directory / PICTURE).is_file():
            raise FileNotFoundError(directory / PICTURE)

    layout, header, _ = decode_ftu(MAIN_FTU)
    button = find_caption(layout, "Button45")
    if button.get("id") != 20080:
        raise RuntimeError(f"unexpected Button45 id: {button.get('id')}")
    button["picTab"] = {"pic0": PICTURE, "pic1": PICTURE, "pic2": PICTURE}
    MAIN_FTU.write_bytes(encode_ftu(layout, header))
    print("set Button45 delete-device artwork")


if __name__ == "__main__":
    main()
