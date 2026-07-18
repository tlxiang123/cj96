#!/usr/bin/env python3
"""Build the largest square address label that does not overlap Window10's list."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
SOURCE = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-0cfe0a53-04c4-4d3f-b77c-f1b45febb77e.png")
OUTPUT = RESOURCE_DIR / "w2_set_address_label_77x77.png"


def extract_blue(source: Image.Image, box: tuple[int, int, int, int]) -> Image.Image:
    crop = source.crop(box).convert("RGBA")
    pixels = crop.load()
    for y in range(crop.height):
        for x in range(crop.width):
            red, green, blue, _ = pixels[x, y]
            saturation = max(red, green, blue) - min(red, green, blue)
            is_blue_art = red < 145 and blue > red + 38 and blue > green
            alpha = max(0, min(255, round((saturation - 20) * 4.5))) if is_blue_art else 0
            pixels[x, y] = (red, green, blue, alpha)
    return crop


def main() -> None:
    source = Image.open(SOURCE).convert("RGB")
    window = Image.open(RESOURCE_DIR / "w2_window10_730x397.png").convert("RGB")
    result = window.crop((43, 18, 120, 95)).convert("RGBA")

    icon = extract_blue(source, (86, 56, 171, 163)).resize((49, 52), Image.Resampling.LANCZOS)
    text = extract_blue(source, (66, 179, 199, 217)).resize((75, 22), Image.Resampling.LANCZOS)
    result.alpha_composite(icon, (14, 0))
    result.alpha_composite(text, (1, 54))
    result.convert("RGB").save(OUTPUT, optimize=True)
    print(f"created {OUTPUT.name} at {result.size[0]}x{result.size[1]}")


if __name__ == "__main__":
    main()
