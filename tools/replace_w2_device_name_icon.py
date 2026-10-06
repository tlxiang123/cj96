#!/usr/bin/env python3
"""Replace only the icon in the Window2 device-name label asset."""

from __future__ import annotations

import shutil
from datetime import datetime
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path(
    r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-62b0eb42-2429-4a59-9f72-bf09a8972f14.png"
)
TARGET = ROOT / "resources" / "w2_set_device_name_label_77x77.png"
ICON_HEIGHT = 50
ICON_REGION_HEIGHT = 53


def extract_blue_icon(path: Path) -> Image.Image:
    source = Image.open(path).convert("RGBA")
    rgb = np.asarray(source.convert("RGB"), dtype=np.float32)
    border = np.concatenate(
        [
            rgb[:12, :, :].reshape(-1, 3),
            rgb[-12:, :, :].reshape(-1, 3),
            rgb[:, :12, :].reshape(-1, 3),
            rgb[:, -12:, :].reshape(-1, 3),
        ],
        axis=0,
    )
    background = np.median(border, axis=0)
    red, green, blue = rgb[:, :, 0], rgb[:, :, 1], rgb[:, :, 2]
    chroma = np.max(rgb, axis=2) - np.min(rgb, axis=2)
    distance = np.linalg.norm(rgb - background, axis=2)
    blue_art = (blue > red + 24) & (blue > green - 18) & (blue > 95) & (chroma > 24)
    alpha = np.clip((distance - 7) * 4.0, 0, 255)
    alpha = np.where(blue_art, alpha, 0).astype(np.uint8)
    alpha = np.asarray(
        Image.fromarray(alpha, "L").filter(ImageFilter.GaussianBlur(0.35)),
        dtype=np.float32,
    )
    premultiplied = rgb - background * (1.0 - alpha[:, :, None] / 255.0)
    foreground = np.where(
        alpha[:, :, None] > 1,
        premultiplied / np.maximum(alpha[:, :, None] / 255.0, 0.01),
        rgb,
    )
    foreground = np.clip(foreground, 0, 255).astype(np.uint8)
    icon = Image.fromarray(np.dstack([foreground, alpha.astype(np.uint8)]), "RGBA")
    bbox = icon.getbbox()
    if bbox is None:
        raise RuntimeError("source image has no blue icon content")
    return icon.crop(bbox)


def clear_old_icon(base: Image.Image) -> Image.Image:
    output = base.copy()
    pixels = np.asarray(base.convert("RGB"), dtype=np.float32)
    red, green, blue = pixels[:, :, 0], pixels[:, :, 1], pixels[:, :, 2]
    blue_mask = (blue > red + 22) & (blue > green - 18) & (blue > 95)

    for y in range(ICON_REGION_HEIGHT):
        row = pixels[y]
        valid = ~blue_mask[y]
        background = np.median(row[valid], axis=0) if np.any(valid) else np.median(pixels[:3], axis=(0, 1))
        for x in range(output.width):
            output.putpixel((x, y), tuple(int(value) for value in background) + (255,))
    return output


def main() -> None:
    if not SOURCE.exists():
        raise FileNotFoundError(SOURCE)
    if not TARGET.exists():
        raise FileNotFoundError(TARGET)

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup = ROOT / "backups" / f"w2_device_name_icon_before_{stamp}.png"
    backup.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(TARGET, backup)

    base = Image.open(TARGET).convert("RGBA")
    if base.size != (77, 77):
        raise RuntimeError(f"unexpected target size: {base.size}")
    icon = extract_blue_icon(SOURCE)
    icon = icon.resize(
        (round(icon.width * ICON_HEIGHT / icon.height), ICON_HEIGHT),
        Image.Resampling.LANCZOS,
    )
    output = clear_old_icon(base)
    output.alpha_composite(icon, ((output.width - icon.width) // 2, 0))
    output.convert("RGB").save(TARGET, optimize=True)

    print(f"updated: {TARGET}")
    print(f"canvas: {output.width}x{output.height}")
    print(f"visible icon: {icon.width}x{icon.height}")
    print(f"backup: {backup}")


if __name__ == "__main__":
    main()
