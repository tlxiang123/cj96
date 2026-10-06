from __future__ import annotations

import argparse
import shutil
from datetime import datetime
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw
from PIL import ImageFilter


WORKSPACE = Path(__file__).resolve().parents[1]
RESOURCE_DIR = WORKSPACE / "resources"
TEMP_DIR = Path(r"C:\Users\Administrator\AppData\Local\Temp")

TARGETS = {
    "time": ("codex-clipboard-4dcfae52-e5ba-4e20-a890-fd1a92745dfe.png", "topset_time_113.png", None),
    "settings": ("codex-clipboard-114f5285-4b87-41a9-81a0-ac15ab691875.png", "topset_settings_header_clean_158x108.png", None),
    "wifi": ("codex-clipboard-25e15898-920d-4674-b4cc-d2e8bef6e52a.png", "topset_wifi_113.png", None),
    "ethernet": ("codex-clipboard-b770b2ee-4f73-40eb-93bc-9f9cd116cdb4.png", "topset_ethernet_113.png", None),
    "display": ("codex-clipboard-ce0b4585-6a48-47bc-8af7-d09450c74abf.png", "topset_display_113.png", None),
    "language": ("codex-clipboard-f9cc677f-9f3b-4e50-840e-df3d9cf7bb78.png", "topset_language_113.png", None),
    "debug": ("codex-clipboard-61b71631-e681-40b4-96e7-92204ade9ca3.png", "topset_debug_113.png", (25, 35, 165, 205)),
    "remote_upgrade": ("codex-clipboard-61b71631-e681-40b4-96e7-92204ade9ca3.png", "topset_remote_upgrade_113.png", (205, 35, 350, 205)),
}

CANVAS_SIZE = (100, 100)
MAX_VISIBLE_EDGE = 80


def read_source(filename: str, crop: tuple[int, int, int, int] | None) -> Image.Image:
    image = Image.open(TEMP_DIR / filename).convert("RGB")
    return image.crop(crop) if crop else image


def remove_background(image: Image.Image) -> Image.Image:
    values = np.asarray(image, dtype=np.float32)
    red, green, blue = values[:, :, 0], values[:, :, 1], values[:, :, 2]
    blue_dominance = blue - red

    strong_blue = (
        (blue >= 130)
        & (blue_dominance >= 35)
        & ((blue - green) >= 8)
    )
    if not np.any(strong_blue):
        raise ValueError("source image does not contain a measurable blue icon")

    candidate = (
        (blue >= 95)
        & (blue_dominance >= 15)
        & ((blue - green) >= 4)
    )
    selected = strong_blue.copy()
    for _ in range(4):
        expanded = Image.fromarray((selected * 255).astype(np.uint8)).filter(
            ImageFilter.MaxFilter(5)
        )
        selected |= candidate & (np.asarray(expanded) > 0)

    foreground_red = float(np.percentile(red[strong_blue], 5))
    red_alpha = (255.0 - red) / max(1.0, 255.0 - foreground_red)
    color_alpha = (blue_dominance - 8.0) / 42.0
    alpha = np.clip(np.minimum(red_alpha, color_alpha), 0.0, 1.0)

    alpha[~selected] = 0.0
    alpha[alpha < 0.035] = 0.0

    safe_alpha = np.maximum(alpha, 0.001)[:, :, None]
    corrected = (values - (1.0 - safe_alpha) * 255.0) / safe_alpha
    corrected = np.clip(corrected, 0.0, 255.0).astype(np.uint8)

    rgba = np.dstack((corrected, np.round(alpha * 255.0).astype(np.uint8)))
    result = Image.fromarray(rgba, "RGBA")
    bbox = result.getchannel("A").getbbox()
    if bbox is None:
        raise ValueError("background removal left no visible pixels")
    return result.crop(bbox)


def fit_visible_content(content: Image.Image) -> Image.Image:
    scale = MAX_VISIBLE_EDGE / max(content.size)
    size = (
        max(1, round(content.width * scale)),
        max(1, round(content.height * scale)),
    )
    return content.resize(size, Image.Resampling.LANCZOS)


def make_icon(source: Image.Image) -> Image.Image:
    content = fit_visible_content(remove_background(source))
    canvas = Image.new("RGBA", CANVAS_SIZE, (0, 0, 0, 0))
    position = (
        (CANVAS_SIZE[0] - content.width) // 2,
        (CANVAS_SIZE[1] - content.height) // 2,
    )
    canvas.alpha_composite(content, position)
    return canvas


def visible_bbox(image: Image.Image, threshold: int = 8) -> tuple[int, int, int, int] | None:
    alpha = np.asarray(image.getchannel("A"))
    mask = Image.fromarray(np.where(alpha >= threshold, 255, 0).astype(np.uint8))
    return mask.getbbox()


def save_contact_sheet(images: list[tuple[str, Image.Image]], path: Path) -> None:
    scale = 4
    gap = 24
    label_height = 30
    cell_width = 100 * scale
    cell_height = 100 * scale + label_height
    sheet = Image.new("RGB", (cell_width * 4 + gap * 3, cell_height * 2 + gap), (235, 238, 242))
    draw = ImageDraw.Draw(sheet)

    for index, (name, image) in enumerate(images):
        x = (index % 4) * (cell_width + gap)
        y = (index // 4) * (cell_height + gap)
        enlarged = image.resize((cell_width, cell_width), Image.Resampling.NEAREST)
        checker = Image.new("RGBA", enlarged.size, (255, 255, 255, 255))
        checker.alpha_composite(enlarged)
        sheet.paste(checker.convert("RGB"), (x, y))
        draw.text((x + 5, y + cell_width + 5), name, fill=(20, 20, 20))

    path.parent.mkdir(parents=True, exist_ok=True)
    sheet.save(path)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_dir = args.output_dir or WORKSPACE / "backups" / f"page1_icons_rebuild_{stamp}"
    output_dir.mkdir(parents=True, exist_ok=True)

    results: list[tuple[str, Image.Image]] = []
    for name, (source_name, target_name, crop) in TARGETS.items():
        result = make_icon(read_source(source_name, crop))
        result.save(output_dir / target_name)
        results.append((name, result))

    save_contact_sheet(results, output_dir / "page1_icons_compare.png")

    if args.apply:
        backup_dir = WORKSPACE / "backups" / f"page1_icons_before_rebuild_{stamp}"
        backup_dir.mkdir(parents=True, exist_ok=True)
        for _, (_, target_name, _) in TARGETS.items():
            target = RESOURCE_DIR / target_name
            if target.exists():
                shutil.copy2(target, backup_dir / target.name)
        for _, (_, target_name, _) in TARGETS.items():
            shutil.copy2(output_dir / target_name, RESOURCE_DIR / target_name)
        print(f"backup={backup_dir}")
        print(f"applied={RESOURCE_DIR}")

    for name, image in results:
        print(name, "canvas", image.size, "visible", visible_bbox(image))
    print(f"preview={output_dir / 'page1_icons_compare.png'}")


if __name__ == "__main__":
    main()
