#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
DESKTOP = Path(r"C:\Users\Administrator\Desktop")
OUTPUT_DIRS = (ROOT / "resources", ROOT / "ui")
BACKGROUND = ROOT / "resources" / "window_info_panel.png"


ICONS = (
    ("filter_icon_48.png", (128, 16)),
    ("water_icon_48.png", (128, 102)),
)


def extract_blue_foreground(src: Image.Image) -> Image.Image:
    src = src.convert("RGBA")
    out = Image.new("RGBA", src.size, (0, 0, 0, 0))
    pixels = src.load()
    out_pixels = out.load()
    for y in range(src.height):
        for x in range(src.width):
            r, g, b, a = pixels[x, y]
            if a == 0:
                continue
            blue_dominant = (b - r >= 28) and (b - g >= 8)
            saturated_blue = (b >= 150) and (g >= 90) and (r <= 170)
            if blue_dominant or saturated_blue:
                alpha = max(0, min(255, a))
                out_pixels[x, y] = (r, g, b, alpha)
    return out


def main() -> None:
    bg = Image.open(BACKGROUND).convert("RGBA")
    for name, (left, top) in ICONS:
        source = DESKTOP / name
        if not source.exists():
            source = ROOT / "resources" / name
        icon = extract_blue_foreground(Image.open(source))
        canvas = bg.crop((left, top, left + 48, top + 48)).convert("RGBA")
        canvas.alpha_composite(icon)
        for out_dir in OUTPUT_DIRS:
            out_dir.mkdir(parents=True, exist_ok=True)
            canvas.save(out_dir / name)
        print(f"wrote {name}")


if __name__ == "__main__":
    main()
