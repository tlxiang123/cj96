#!/usr/bin/env python3
"""Add one fixed Window2 address pin icon while keeping address digits as text."""

from __future__ import annotations

from pathlib import Path

import numpy as np
from PIL import Image, ImageFilter

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
SOURCE = Path(
    r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-1f0065cb-e64f-4471-9577-79ab4365f6f2.png"
)
ICON_NAME = "w2_address_pin_fixed_50x50.png"
ICON_KEY = "textview__370"
ICON_ID = 50350
ICON_POS = {"height": 50, "left": 73, "top": 2, "width": 50}


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
    alpha_img = Image.fromarray(alpha, "L").filter(ImageFilter.GaussianBlur(0.35))
    alpha = np.asarray(alpha_img, dtype=np.float32)
    premult = rgb - background * (1.0 - alpha[:, :, None] / 255.0)
    foreground = np.where(alpha[:, :, None] > 1, premult / np.maximum(alpha[:, :, None] / 255.0, 0.01), rgb)
    foreground = np.clip(foreground, 0, 255).astype(np.uint8)
    icon = Image.fromarray(np.dstack([foreground, alpha.astype(np.uint8)]), "RGBA")
    bbox = icon.getbbox()
    if bbox is None:
        raise RuntimeError("source icon extraction produced no visible pixels")
    return icon.crop(bbox)


def build_icon() -> None:
    if not SOURCE.exists():
        raise FileNotFoundError(SOURCE)
    canvas = Image.new("RGBA", (ICON_POS["width"], ICON_POS["height"]), (0, 0, 0, 0))
    icon = extract_blue_icon(SOURCE)
    target_h = 50
    target_w = round(icon.width * target_h / icon.height)
    icon = icon.resize((target_w, target_h), Image.Resampling.LANCZOS)
    canvas.alpha_composite(icon, ((canvas.width - icon.width) // 2, 0))
    output = ROOT / "resources" / ICON_NAME
    output.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output, optimize=True)
    print(f"created {ICON_NAME}, visible icon {target_w}x{target_h}, canvas 50x50")


def update_ftu() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    region = find_caption(layout, "W2SetRegion1Window")
    existing = None
    for key, value in region.items():
        if isinstance(value, dict) and value.get("caption") == "W2AddressFixedIcon":
            existing = key
            break
    if existing and existing != ICON_KEY:
        region.pop(existing)

    region[ICON_KEY] = {
        "alignment": 37,
        "backgroundPic": ICON_NAME,
        "caption": "W2AddressFixedIcon",
        "colorTab": {"color0": 16777215},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 20,
        "id": ICON_ID,
        "position": dict(ICON_POS),
        "touchable": False,
    }

    label = find_caption(layout, "TextView1")
    label["position"] = {"height": 24, "left": 56, "top": 53, "width": 84}
    label["text"] = "设备地址"
    label.pop("backgroundPic", None)

    UI_PATH.write_bytes(encode_ftu(layout, header))
    print("updated ui/main.ftu")


def main() -> None:
    build_icon()
    update_ftu()


if __name__ == "__main__":
    main()
