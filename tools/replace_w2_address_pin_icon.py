#!/usr/bin/env python3
"""Replace the Window2 address label pin inside all combined address images."""

from __future__ import annotations

import shutil
from datetime import datetime
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path(
    r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-1f0065cb-e64f-4471-9577-79ab4365f6f2.png"
)
TARGET_PATTERN = "w2_set_address_combined_*.png"
ICON_AREA = (0, 0, 77, 54)
ICON_TARGET_HEIGHT = 50
ICON_TOP = 1
BACKGROUND_CANDIDATES = [
    ROOT / "Release" / "sdcard_runtime_20260731_152311" / "resources" / "w2_window10_730x397.png",
    ROOT / "backups" / "ui_full_20260720_131013" / "resources" / "w2_window10_730x397.png",
    ROOT / "backups" / "board_to_pc_sync_20260812_093718" / "resources" / "w2_window10_730x397.png",
    ROOT / "backups" / "resources_cleanup_20260912_085127" / "removed" / "w2_window10_730x397.png",
]


def find_background() -> Image.Image:
    for path in BACKGROUND_CANDIDATES:
        if path.exists():
            return Image.open(path).convert("RGBA").crop((43, 18, 242, 95))
    raise FileNotFoundError("w2_window10_730x397.png backup was not found")


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
    alpha_img = Image.fromarray(alpha, "L").filter(ImageFilter.GaussianBlur(0.35))
    alpha = np.asarray(alpha_img, dtype=np.float32)
    premult = rgb - background * (1.0 - alpha[:, :, None] / 255.0)
    foreground = np.where(alpha[:, :, None] > 1, premult / np.maximum(alpha[:, :, None] / 255.0, 0.01), rgb)
    foreground = np.clip(foreground, 0, 255).astype(np.uint8)
    rgba = np.dstack([foreground, alpha.astype(np.uint8)])
    icon = Image.fromarray(rgba, "RGBA")
    bbox = icon.getbbox()
    if bbox is None:
        raise RuntimeError("source icon extraction produced no visible pixels")
    return icon.crop(bbox)


def fit_icon(icon: Image.Image) -> Image.Image:
    width = round(icon.width * ICON_TARGET_HEIGHT / icon.height)
    return icon.resize((width, ICON_TARGET_HEIGHT), Image.Resampling.LANCZOS)


def backup_targets(targets: list[Path]) -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_root = ROOT / "backups" / f"w2_address_pin_replace_{stamp}"
    for target in targets:
        relative = target.relative_to(ROOT)
        backup_path = backup_root / relative
        backup_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(target, backup_path)
    return backup_root


def replace_icon(target: Path, clean_area: Image.Image, icon: Image.Image) -> None:
    canvas = Image.open(target).convert("RGBA")
    canvas.alpha_composite(clean_area, (0, 0))
    x = (77 - icon.width) // 2
    canvas.alpha_composite(icon, (x, ICON_TOP))
    canvas.convert("RGB").save(target, optimize=True)


def main() -> None:
    if not SOURCE.exists():
        raise FileNotFoundError(SOURCE)
    targets: list[Path] = []
    for folder in [ROOT / "resources", ROOT / "ui"]:
        targets.extend(sorted(folder.glob(TARGET_PATTERN)))
    if not targets:
        raise RuntimeError("no combined address images were found")

    backup_root = backup_targets(targets)
    background = find_background()
    clean_area = background.crop(ICON_AREA)
    icon = fit_icon(extract_blue_icon(SOURCE))
    for target in targets:
        replace_icon(target, clean_area, icon)
    print(f"updated {len(targets)} files")
    print(f"backup: {backup_root}")
    print(f"icon visible size: {icon.width}x{icon.height}")
    print("sample: resources/w2_set_address_combined_001.png")


if __name__ == "__main__":
    main()
