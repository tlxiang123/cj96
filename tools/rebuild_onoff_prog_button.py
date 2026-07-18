#!/usr/bin/env python3
"""Redraw OnOffProgButton at its native 65x65 icon size."""

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
UI_PATH = ROOT / "ui" / "main.ftu"


def build_toggle(path: Path, selected: bool) -> None:
    scale = 4
    image = Image.new("RGBA", (65 * scale, 65 * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    track = (52, 199, 89, 255) if selected else (83, 100, 117, 255)
    outline = (29, 138, 80, 255) if selected else (0, 91, 187, 255)
    draw.rounded_rectangle(
        (1 * scale, 17 * scale, 63 * scale, 47 * scale),
        radius=15 * scale,
        fill=track,
        outline=outline,
        width=1 * scale,
    )
    knob_left = 34 if selected else 4
    draw.ellipse(
        (knob_left * scale, 20 * scale, (knob_left + 27) * scale, 47 * scale),
        fill=(255, 250, 240, 255),
    )
    image.resize((65, 65), Image.Resampling.LANCZOS).save(
        path, format="PNG", optimize=True
    )


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


off = RESOURCES / "window3_program_toggle_off_65.png"
on = RESOURCES / "window3_program_toggle_on_65.png"
build_toggle(off, False)
build_toggle(on, True)

layout, header, _ = decode_ftu(UI_PATH)
button = find_caption(layout, "OnOffProgButton")
button["picTab"] = {"pic0": off.name, "pic2": on.name}
UI_PATH.write_bytes(encode_ftu(layout, header))
print("rebuilt OnOffProgButton without changing Window3ScrollWindow colors")
