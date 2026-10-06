#!/usr/bin/env python3
"""Replace Window2 action button art with tight transparent icon-only assets."""

from __future__ import annotations

import shutil
import sys
from datetime import datetime
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
UI_DIR = ROOT / "ui"
ICON_BOX = (72, 58)

SOURCES = [
    (
        Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-3ea33e0f-dc00-41d7-9627-d59ea2312ab3.png"),
        "w2_set_bind_113x113_norm.png",
        20014,
    ),
    (
        Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-83632004-57bb-4fc7-a961-fa521fb6f2ae.png"),
        "w2_set_clear_113x113_norm.png",
        20006,
    ),
    (
        Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-45fa1e76-29d1-4f3c-9ec6-8821489dd5ae.png"),
        "w2_set_delete_group_113x113_norm.png",
        20012,
    ),
    (
        Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-4f3e9cf0-c551-4007-9a25-0aaca780828f.png"),
        "w2_set_rename_group_113x113_norm.png",
        20078,
    ),
]


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
    alpha = np.where(blue_art, np.clip((distance - 7) * 4.0, 0, 255), 0).astype(np.uint8)
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
    icon = Image.fromarray(
        np.dstack([np.clip(foreground, 0, 255).astype(np.uint8), alpha.astype(np.uint8)]),
        "RGBA",
    )
    bbox = icon.getchannel("A").getbbox()
    if bbox is None:
        raise RuntimeError(f"no blue icon content extracted from {path}")
    return icon.crop(bbox)


def fit_icon(icon: Image.Image) -> Image.Image:
    scale = min(ICON_BOX[0] / icon.width, ICON_BOX[1] / icon.height)
    size = (round(icon.width * scale), round(icon.height * scale))
    return icon.resize(size, Image.Resampling.LANCZOS)


def find_control(node: object, control_id: int) -> dict | None:
    if isinstance(node, dict):
        if node.get("id") == control_id:
            return node
        for value in node.values():
            result = find_control(value, control_id)
            if result is not None:
                return result
    elif isinstance(node, list):
        for value in node:
            result = find_control(value, control_id)
            if result is not None:
                return result
    return None


def backup_targets() -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_root = ROOT / "backups" / f"w2_action_button_icons_before_{stamp}"
    backup_root.mkdir(parents=True, exist_ok=True)
    for _, output_name, _ in SOURCES:
        target = RESOURCE_DIR / output_name
        if target.exists():
            shutil.copy2(target, backup_root / output_name)
    shutil.copy2(UI_DIR / "main.ftu", backup_root / "main.ftu")
    return backup_root


def main() -> None:
    for source, _, _ in SOURCES:
        if not source.exists():
            raise FileNotFoundError(source)

    backup_root = backup_targets()
    ftu_path = UI_DIR / "main.ftu"
    layout, header, _ = decode_ftu(ftu_path)

    for source, output_name, control_id in SOURCES:
        target = RESOURCE_DIR / output_name
        old_offset = (0, 0)
        if target.exists():
            old_bbox = Image.open(target).convert("RGBA").getchannel("A").getbbox()
            if old_bbox is not None:
                old_offset = (old_bbox[0], old_bbox[1])

        icon = fit_icon(extract_blue_icon(source))
        icon.save(target, optimize=True)

        control = find_control(layout, control_id)
        if control is None:
            raise RuntimeError(f"control id {control_id} not found")
        position = control["position"]
        position["left"] += old_offset[0]
        position["top"] += old_offset[1]
        position["width"] = icon.width
        position["height"] = icon.height
        control.pop("bgColorTab", None)

        print(
            f"{output_name}: canvas={icon.width}x{icon.height}, "
            f"position=({position['left']},{position['top']},{position['width']},{position['height']})"
        )

    ftu_path.write_bytes(encode_ftu(layout, header))
    print(f"backup: {backup_root}")
    print(f"ui_png_count: {len(list((ROOT / 'ui').glob('*.png')))}")


if __name__ == "__main__":
    main()
