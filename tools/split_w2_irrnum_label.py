#!/usr/bin/env python3
"""Split Window2 selected-group label/value assets into icon and live text."""

from __future__ import annotations

import shutil
from datetime import datetime
from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
SOURCE = Path(
    r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-036572cf-99ad-493e-9dcf-30e918e2f2fc.png"
)
ICON_NAME = "w2_irrnum_fixed_icon_113x69.png"
BLUE = 0x005BBB


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    raise LookupError(caption)


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


def backup_current() -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_root = ROOT / "backups" / f"w2_irrnum_split_before_{stamp}"
    backup_root.mkdir(parents=True, exist_ok=True)
    for path in [
        UI_PATH,
        ROOT / "resources" / "w2_set_irr_label_113x69_v2.png",
    ]:
        if path.exists():
            shutil.copy2(path, backup_root / path.name)
    for path in sorted((ROOT / "resources").glob("w2_set_irr_value*.png")):
        shutil.copy2(path, backup_root / path.name)
    return backup_root


def build_icon() -> tuple[int, int]:
    icon = extract_blue_icon(SOURCE)
    scale = min(113 / icon.width, 69 / icon.height)
    visible_size = (round(icon.width * scale), round(icon.height * scale))
    icon = icon.resize(visible_size, Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", (113, 69), (0, 0, 0, 0))
    canvas.alpha_composite(icon, ((canvas.width - icon.width) // 2, (canvas.height - icon.height) // 2))
    output = ROOT / "resources" / ICON_NAME
    output.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output, optimize=True)
    return visible_size


def update_ftu() -> None:
    layout, header, _ = decode_ftu(UI_PATH)

    label = find_caption(layout, "IrrNum_TextView")
    label["backgroundPic"] = ICON_NAME
    label.pop("bgColorTab", None)
    label.pop("text", None)
    label["colorTab"] = {"color0": 16777215}
    label["fontSize"] = 20
    label["alignment"] = 37
    label["touchable"] = False

    value = find_caption(layout, "IrrNumValue_TextView")
    value.pop("backgroundPic", None)
    value.pop("bgColorTab", None)
    value["text"] = "---"
    value["colorTab"] = {"color0": BLUE}
    value["fontSize"] = 32
    value["alignment"] = 37
    value["touchable"] = False

    UI_PATH.write_bytes(encode_ftu(layout, header))


def move_old_images(backup_root: Path) -> int:
    moved = 0
    old_files = [ROOT / "resources" / "w2_set_irr_label_113x69_v2.png"]
    old_files.extend(sorted((ROOT / "resources").glob("w2_set_irr_value*.png")))
    removed_root = backup_root / "removed"
    removed_root.mkdir(exist_ok=True)
    for path in old_files:
        if not path.exists():
            continue
        destination = removed_root / path.name
        if destination.exists():
            destination.unlink()
        shutil.move(str(path), str(destination))
        moved += 1
    return moved


def main() -> None:
    if not SOURCE.exists():
        raise FileNotFoundError(SOURCE)
    backup_root = backup_current()
    visible_size = build_icon()
    update_ftu()
    moved = move_old_images(backup_root)
    print(f"created resources/{ICON_NAME}")
    print(f"canvas: 113x69")
    print(f"visible icon: {visible_size[0]}x{visible_size[1]}")
    print("updated ui/main.ftu")
    print(f"moved old irrnum images: {moved}")
    print(f"backup: {backup_root}")


if __name__ == "__main__":
    main()
