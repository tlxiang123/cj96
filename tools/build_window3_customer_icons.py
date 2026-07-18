#!/usr/bin/env python3
"""Extract the supplied Window3 artwork without changing its blue color."""

from __future__ import annotations

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "resources" / "window3_icon_sources"
RESOURCE_DIR = ROOT / "resources"

# Crop away the screenshot frame and browser overlay while keeping the
# supplied artwork color. Each supplied page icon retains its own caption.
ICON_SPECS = {
    "program": ((75, 35, 215, 205), "window3_program_88.png", 88),
    "enable": ((98, 45, 210, 218), "window3_enable_88.png", 88),
    "start_time": ((90, 50, 220, 225), "window3_start_time_108.png", 108),
    "week_mode": ((95, 30, 250, 225), "window3_week_mode_108.png", 108),
    "interval_mode": ((55, 40, 225, 230), "window3_interval_mode_100.png", 100),
    "interval": ((70, 55, 245, 205), "window3_interval_96.png", 96),
    "delay": ((88, 30, 222, 205), "window3_delay_96.png", 96),
}


def extract_blue_art(
    source: Image.Image,
    crop: tuple[int, int, int, int],
    output_size: int,
) -> Image.Image:
    image = source.convert("RGB").crop(crop)
    transparent = Image.new("RGBA", image.size, (0, 0, 0, 0))
    source_pixels = image.load()
    output_pixels = transparent.load()
    for y in range(image.height):
        for x in range(image.width):
            red, green, blue = source_pixels[x, y]
            # Coverage is derived from the supplied blue-vs-red difference.
            # This retains antialiased edge pixels without recoloring them.
            alpha = max(0, min(255, round((blue - red - 2) * 1.65)))
            if alpha:
                output_pixels[x, y] = (red, green, blue, alpha)

    transparent.thumbnail((output_size, output_size), Image.Resampling.LANCZOS)
    output = Image.new("RGBA", (output_size, output_size), (0, 0, 0, 0))
    output.alpha_composite(
        transparent,
        (
            (output_size - transparent.width) // 2,
            (output_size - transparent.height) // 2,
        ),
    )
    return output


def find_spray_source() -> Path:
    candidates = [
        path for path in RESOURCE_DIR.glob("*60x60.png")
        if path.name != "window3_spray_program_60.png"
    ]
    if not candidates:
        raise FileNotFoundError("the saved 60x60 spray program image was not found")
    return candidates[0]


def remove_white_from_spray(source: Image.Image) -> Image.Image:
    image = source.convert("RGB")
    output = Image.new("RGBA", image.size, (0, 0, 0, 0))
    source_pixels = image.load()
    output_pixels = output.load()
    for y in range(image.height):
        for x in range(image.width):
            red, green, blue = source_pixels[x, y]
            alpha = max(0, min(255, round((blue - red - 2) * 1.65)))
            if alpha:
                output_pixels[x, y] = (red, green, blue, alpha)
    return output


def main() -> None:
    for stem, (crop, filename, output_size) in ICON_SPECS.items():
        source = Image.open(SOURCE_DIR / f"{stem}.png")
        extract_blue_art(source, crop, output_size).save(
            RESOURCE_DIR / filename, optimize=True
        )

    spray = Image.open(find_spray_source()).convert("RGBA")
    if spray.size != (60, 60):
        spray = spray.resize((60, 60), Image.Resampling.LANCZOS)
    remove_white_from_spray(spray).save(
        RESOURCE_DIR / "window3_spray_program_60.png", optimize=True
    )
    print("built Window3 customer icons without recoloring")


if __name__ == "__main__":
    main()
