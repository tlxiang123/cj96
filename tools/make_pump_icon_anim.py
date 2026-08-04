#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "resources" / "window7_pump_icon.png"
OUTPUT_DIRS = (ROOT / "resources", ROOT / "ui")
FRAME_COUNT = 20

# Keep the round pump housing fixed. Only the inner impeller is rotated.
IMPELLER_BOX = (4, 12, 20, 28)
SUPERSAMPLE = 4


def make_soft_circle_mask(size: tuple[int, int]) -> Image.Image:
    high_size = (size[0] * SUPERSAMPLE, size[1] * SUPERSAMPLE)
    mask = Image.new("L", high_size, 0)
    draw = ImageDraw.Draw(mask)
    inset = SUPERSAMPLE
    draw.ellipse(
        (inset, inset, high_size[0] - inset - 1, high_size[1] - inset - 1),
        fill=255,
    )
    return mask.resize(size, Image.Resampling.LANCZOS)


def main() -> None:
    base = Image.open(SOURCE).convert("RGBA")
    impeller = base.crop(IMPELLER_BOX)
    mask = make_soft_circle_mask(impeller.size)

    for i in range(FRAME_COUNT):
        angle = -360.0 * i / FRAME_COUNT
        frame = base.copy()

        large = impeller.resize(
            (impeller.width * SUPERSAMPLE, impeller.height * SUPERSAMPLE),
            Image.Resampling.LANCZOS,
        )
        rotated = large.rotate(angle, resample=Image.Resampling.BICUBIC, expand=False)
        rotated = rotated.resize(impeller.size, Image.Resampling.LANCZOS)
        frame.paste(rotated, IMPELLER_BOX[:2], mask)

        name = f"window7_pump_icon_anim_{i:02d}.png"
        for out_dir in OUTPUT_DIRS:
            out_dir.mkdir(parents=True, exist_ok=True)
            frame.save(out_dir / name)

    print(f"generated {FRAME_COUNT} pump icon animation frames")


if __name__ == "__main__":
    main()
