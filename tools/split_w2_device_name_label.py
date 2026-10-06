#!/usr/bin/env python3
"""Split the Window2 device-name header into a fixed icon and editable text."""

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
    r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-62b0eb42-2429-4a59-9f72-bf09a8972f14.png"
)
ICON_NAME = "w2_device_name_fixed_icon_50x50.png"
ICON_KEY = "textview__368"
ICON_ID = 50351
ICON_POS = {"height": 50, "left": 317, "top": 6, "width": 50}
LABEL_POS = {"height": 24, "left": 299, "top": 58, "width": 86}
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


def build_icon() -> tuple[int, int]:
    icon = extract_blue_icon(SOURCE)
    icon = icon.resize(
        (round(icon.width * ICON_POS["height"] / icon.height), ICON_POS["height"]),
        Image.Resampling.LANCZOS,
    )
    canvas = Image.new("RGBA", (ICON_POS["width"], ICON_POS["height"]), (0, 0, 0, 0))
    canvas.alpha_composite(icon, ((canvas.width - icon.width) // 2, 0))
    output = ROOT / "resources" / ICON_NAME
    output.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output, optimize=True)
    return icon.size


def update_ftu() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    region = find_caption(layout, "W2SetRegion1Window")

    for key, value in list(region.items()):
        if isinstance(value, dict) and value.get("caption") == "W2DeviceNameFixedIcon":
            region.pop(key)

    label = find_caption(region, "TextView3")
    label.pop("backgroundPic", None)
    label.pop("bgColorTab", None)
    label["text"] = "设备名称"
    label["position"] = dict(LABEL_POS)
    label["colorTab"] = {"color0": BLUE}
    label["fontSize"] = 20
    label["alignment"] = 37
    label["touchable"] = False

    region[ICON_KEY] = {
        "alignment": 37,
        "backgroundPic": ICON_NAME,
        "caption": "W2DeviceNameFixedIcon",
        "colorTab": {"color0": 16777215},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 20,
        "id": ICON_ID,
        "position": dict(ICON_POS),
        "touchable": False,
    }

    UI_PATH.write_bytes(encode_ftu(layout, header))


def backup_current() -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_root = ROOT / "backups" / f"w2_device_name_split_before_{stamp}"
    backup_root.mkdir(parents=True, exist_ok=True)
    for path in [UI_PATH, ROOT / "resources" / "w2_set_device_name_label_77x77.png"]:
        if path.exists():
            shutil.copy2(path, backup_root / path.name)
    return backup_root


def main() -> None:
    if not SOURCE.exists():
        raise FileNotFoundError(SOURCE)
    backup_root = backup_current()
    visible_size = build_icon()
    update_ftu()
    print(f"created resources/{ICON_NAME}")
    print(f"icon visible size: {visible_size[0]}x{visible_size[1]}")
    print("updated ui/main.ftu: TextView3 is now plain text")
    print(f"backup: {backup_root}")


if __name__ == "__main__":
    main()
