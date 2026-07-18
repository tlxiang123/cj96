#!/usr/bin/env python3
"""Align the three address digit cards while preserving their original artwork."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"


def fill_horizontal(image: Image.Image, left: int, right: int, top: int, bottom: int) -> None:
    pixels = image.load()
    sample_left = left - 1
    sample_right = right
    width = right - left
    for y in range(top, bottom):
        a = pixels[sample_left, y]
        b = pixels[sample_right, y]
        for offset, x in enumerate(range(left, right), 1):
            ratio = offset / (width + 1)
            pixels[x, y] = tuple(round(av * (1.0 - ratio) + bv * ratio) for av, bv in zip(a, b))


def align(path: Path) -> None:
    original = Image.open(path).convert("RGB")
    result = original.copy()

    # These crops include the existing card edge and glow. Only their x position changes.
    middle = original.crop((37, 8, 68, 62))
    right = original.crop((69, 8, 100, 62))
    fill_horizontal(result, 37, 68, 8, 62)
    fill_horizontal(result, 69, 100, 8, 62)
    result.paste(middle, (36, 8))
    result.paste(right, (66, 8))
    result.save(path, optimize=True)


def main() -> None:
    paths = sorted(RESOURCE_DIR.glob("w2_set_address_value_*.png"))
    if len(paths) != 255:
        raise RuntimeError(f"expected 255 address images, found {len(paths)}")
    for path in paths:
        align(path)
    print(f"aligned {len(paths)} address images")


if __name__ == "__main__":
    main()
