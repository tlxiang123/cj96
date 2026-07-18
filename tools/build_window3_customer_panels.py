#!/usr/bin/env python3
"""Build pixel-matched Window3 region panels with even rounded borders."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
SCALE = 4

# These dimensions match layout_window3_customer.py exactly. Keeping every
# bitmap at its rendered size prevents non-uniform border and radius scaling.
PANEL_SIZES = {
    1: (220, 100),
    2: (470, 100),
    3: (277, 100),
    4: (987, 145),
    5: (488, 290),
    6: (488, 290),
    7: (987, 300),
}

# Preserve the colors used by the current Window3 panels.
TOP_COLOR = (255, 253, 248, 248)
BOTTOM_COLOR = (244, 250, 255, 248)
OUTLINE_COLOR = (73, 174, 238, 255)


def vertical_gradient(size: tuple[int, int]) -> Image.Image:
    width, height = size
    gradient = Image.new("RGBA", size)
    draw = ImageDraw.Draw(gradient)
    for y in range(height):
        ratio = y / max(1, height - 1)
        color = tuple(
            round(TOP_COLOR[channel] + (BOTTOM_COLOR[channel] - TOP_COLOR[channel]) * ratio)
            for channel in range(4)
        )
        draw.line((0, y, width - 1, y), fill=color)
    return gradient


def build_panel(width: int, height: int) -> Image.Image:
    large_size = (width * SCALE, height * SCALE)
    panel = Image.new("RGBA", large_size, (0, 0, 0, 0))
    gradient = vertical_gradient(large_size)

    # Use the same inset on all four sides and draw at 4x resolution. This
    # produces symmetric corners and a consistent two-pixel final outline.
    inset = 2 * SCALE
    box = (
        inset,
        inset,
        large_size[0] - inset - 1,
        large_size[1] - inset - 1,
    )
    radius = 12 * SCALE
    mask = Image.new("L", large_size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(box, radius=radius, fill=255)
    panel.paste(gradient, (0, 0), mask)
    ImageDraw.Draw(panel).rounded_rectangle(
        box,
        radius=radius,
        outline=OUTLINE_COLOR,
        width=2 * SCALE,
    )
    return panel.resize((width, height), Image.Resampling.LANCZOS)


def verify_panel(path: Path, expected_size: tuple[int, int]) -> None:
    image = Image.open(path).convert("RGBA")
    if image.size != expected_size:
        raise RuntimeError(f"wrong panel size: {path.name}: {image.size}")
    alpha = image.getchannel("A")
    width, height = image.size
    corners = (
        alpha.getpixel((0, 0)),
        alpha.getpixel((width - 1, 0)),
        alpha.getpixel((0, height - 1)),
        alpha.getpixel((width - 1, height - 1)),
    )
    if corners != (0, 0, 0, 0):
        raise RuntimeError(f"panel corners are not symmetric: {path.name}: {corners}")


def main() -> None:
    for index, size in PANEL_SIZES.items():
        path = RESOURCE_DIR / f"window3_region_{index}.png"
        build_panel(*size).save(path, optimize=True)
        verify_panel(path, size)
    print("built 7 pixel-matched Window3 panels")


if __name__ == "__main__":
    main()
