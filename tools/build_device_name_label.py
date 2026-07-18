#!/usr/bin/env python3
"""Create the 77x77 TextView3 device-name label from the supplied source art."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
SOURCE = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-c3ee525b-9b4d-42de-a895-900d9c44c7e4.png")
OUTPUT = RESOURCE_DIR / "w2_set_device_name_label_77x77.png"


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
    result = window.crop((280, 18, 357, 95)).convert("RGBA")

    icon = extract_blue(source, (82, 69, 217, 162)).resize((73, 50), Image.Resampling.LANCZOS)
    text = extract_blue(source, (86, 174, 215, 208)).resize((75, 20), Image.Resampling.LANCZOS)
    result.alpha_composite(icon, (2, 0))
    result.alpha_composite(text, (1, 55))
    result.convert("RGB").save(OUTPUT, optimize=True)
    print(f"created {OUTPUT.name} at 77x77")


if __name__ == "__main__":
    main()
