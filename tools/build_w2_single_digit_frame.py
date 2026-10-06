#!/usr/bin/env python3
"""Create a one-digit frame matching the Window2 selected-group number boxes."""

from __future__ import annotations

from pathlib import Path

from PIL import Image
import numpy as np


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "resources" / "w2_set_irr_value_001.png"
OUTPUT = "w2_single_digit_frame_27x39.png"
BOX = (4, 15, 31, 54)

def main() -> None:
    if not SOURCE.exists():
        raise FileNotFoundError(SOURCE)

    crop = Image.open(SOURCE).convert("RGBA").crop(BOX)
    pixels = np.array(crop)
    red, green, blue = pixels[:, :, 0], pixels[:, :, 1], pixels[:, :, 2]
    blue_mask = (
        (blue > 120)
        & (blue > red + 30)
        & (blue > green - 10)
        & ((np.maximum.reduce([red, green, blue]) - np.minimum.reduce([red, green, blue])) > 30)
    )
    height, width = blue_mask.shape
    seen = np.zeros_like(blue_mask, dtype=bool)
    keep = np.zeros_like(blue_mask, dtype=bool)
    for y in range(height):
        for x in range(width):
            if not blue_mask[y, x] or seen[y, x]:
                continue
            stack = [(x, y)]
            seen[y, x] = True
            points = []
            while stack:
                cx, cy = stack.pop()
                points.append((cx, cy))
                for nx in range(cx - 1, cx + 2):
                    for ny in range(cy - 1, cy + 2):
                        if (
                            nx < 0
                            or nx >= width
                            or ny < 0
                            or ny >= height
                            or seen[ny, nx]
                            or not blue_mask[ny, nx]
                        ):
                            continue
                        seen[ny, nx] = True
                        stack.append((nx, ny))
            xs = [point[0] for point in points]
            ys = [point[1] for point in points]
            bbox = (min(xs), min(ys), max(xs) + 1, max(ys) + 1)
            touches_edge = bbox[0] <= 1 or bbox[1] <= 1 or bbox[2] >= width - 1 or bbox[3] >= height - 1
            if touches_edge and len(points) > 80:
                for px, py in points:
                    keep[py, px] = True

    output_pixels = np.zeros_like(pixels)
    output_pixels[keep] = pixels[keep]
    frame = Image.fromarray(output_pixels, "RGBA")
    output_path = ROOT / "resources" / OUTPUT
    output_path.parent.mkdir(parents=True, exist_ok=True)
    frame.save(output_path, optimize=True)

    print(f"created {OUTPUT}")
    print(f"size: {frame.width}x{frame.height}")


if __name__ == "__main__":
    main()
