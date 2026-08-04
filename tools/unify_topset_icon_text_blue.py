from __future__ import annotations

from datetime import datetime
from pathlib import Path
import shutil

import numpy as np
from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
RES = ROOT / "resources"

TARGET_FILES = [
    "topset_wifi_113.png",
    "topset_ethernet_113.png",
    "topset_4g_113.png",
    "topset_time_113.png",
    "topset_language_113.png",
    "topset_display_113.png",
    "topset_back_36.png",
    "topset_settings_header_clean_158x108.png",
]


def debug_foreground_blue() -> np.ndarray:
    """Use the existing Debug button as the color source; do not redraw shapes."""
    arr = np.array(Image.open(RES / "topset_debug_113.png").convert("RGBA"))
    mask = (
        (arr[:, :, 3] > 20)
        & (arr[:, :, 0] < 90)
        & (arr[:, :, 1] < 170)
        & (arr[:, :, 2] > 110)
        & ((arr[:, :, 2].astype(np.int16) - arr[:, :, 0].astype(np.int16)) > 50)
    )
    pixels = arr[:, :, :3][mask]
    if len(pixels) == 0:
        return np.array([1, 91, 187], dtype=np.uint8)
    return np.median(pixels, axis=0).astype(np.uint8)


def recolor_foreground(path: Path, target: np.ndarray) -> int:
    im = Image.open(path).convert("RGBA")
    arr = np.array(im)
    rgb = arr[:, :, :3].astype(np.int16)
    alpha = arr[:, :, 3]

    # Only recolor existing blue/cyan foreground strokes and text.
    # The icon geometry, rounded rectangle, transparency, shadows, and light backgrounds are preserved.
    blueish = (
        (alpha > 15)
        & (rgb[:, :, 2] > 95)
        & ((rgb[:, :, 2] - rgb[:, :, 0]) > 35)
        & ((rgb[:, :, 1] - rgb[:, :, 0]) > -20)
    )
    dark_foreground = (rgb[:, :, 0] < 120) & (rgb[:, :, 1] < 180)
    not_light_fill = ~((rgb[:, :, 0] > 175) & (rgb[:, :, 1] > 210) & (rgb[:, :, 2] > 225))
    mask = blueish & dark_foreground & not_light_fill

    arr[:, :, 0][mask] = target[0]
    arr[:, :, 1][mask] = target[1]
    arr[:, :, 2][mask] = target[2]
    Image.fromarray(arr, "RGBA").save(path)
    return int(mask.sum())


def make_preview(paths: list[Path], out_path: Path) -> None:
    thumbs = []
    for p in paths:
        im = Image.open(p).convert("RGBA")
        canvas = Image.new("RGBA", (160, 130), (255, 255, 255, 255))
        x = (160 - im.width) // 2
        y = (110 - im.height) // 2
        canvas.alpha_composite(im, (x, y))
        draw = ImageDraw.Draw(canvas)
        draw.text((8, 112), p.name[:24], fill=(0, 91, 187, 255))
        thumbs.append(canvas.convert("RGB"))

    cols = 4
    rows = (len(thumbs) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * 160, rows * 130), (245, 250, 255))
    for i, im in enumerate(thumbs):
        sheet.paste(im, ((i % cols) * 160, (i // cols) * 130))
    sheet.save(out_path)


def main() -> None:
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = ROOT / "Release" / f"before_topset_color_unify_{timestamp}"
    backup_dir.mkdir(parents=True, exist_ok=False)

    paths = [RES / name for name in TARGET_FILES]
    for path in paths:
        if not path.exists():
            raise FileNotFoundError(path)
        shutil.copy2(path, backup_dir / path.name)

    target = debug_foreground_blue()
    print(f"target_blue=#{target[0]:02x}{target[1]:02x}{target[2]:02x}")
    for path in paths:
        changed = recolor_foreground(path, target)
        print(f"{path.name}: recolored_pixels={changed}")

    preview_path = ROOT / "Release" / f"topset_color_unify_preview_{timestamp}.png"
    make_preview(paths, preview_path)
    print(f"backup={backup_dir}")
    print(f"preview={preview_path}")


if __name__ == "__main__":
    main()
