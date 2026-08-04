#!/usr/bin/env python3
from __future__ import annotations

import shutil
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageStat


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"

LATEST_REFS = {
    "close_sensor": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-0c22c179-1b58-4ef3-b655-c230eac9b066.png"),
    "solenoid_source": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-30a3ff84-8ae6-4311-891f-e1b9383d920c.png"),
    "target_edit": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-46c8582f-3874-4739-94e8-9834e3b65fb4.png"),
    "test_force": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-a5d8ea5c-0dc2-4c34-b8c4-8dacc4b01cd4.png"),
    # Earlier reference still available in this thread; it contains the same style open-valve button.
    "open_old": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-22c952f4-65e9-4270-9b13-5c7825203e91.png"),
}

# Crop boxes intentionally include the full rounded button surface, not only the icon.
# The final 120x60 resource is produced with Lanczos downsampling and a rounded alpha mask.
CROPS = {
    "Close_Valve.png": ("close_sensor", (70, 28, 526, 200)),
    "Sensor.png": ("close_sensor", (61, 272, 530, 479)),
    "Sensor_Selected.png": ("close_sensor", (61, 272, 530, 479)),
    "Solenoid_Valve.png": ("solenoid_source", (82, 42, 566, 218)),
    "Solenoid_Valve_Selected.png": ("solenoid_source", (82, 42, 566, 218)),
    "Source_Address.png": ("solenoid_source", (82, 293, 559, 494)),
    "Target_Address.png": ("target_edit", (86, 45, 542, 213)),
    "Edit_Address.png": ("target_edit", (84, 301, 543, 490)),
    "Test_Address.png": ("test_force", (94, 42, 554, 198)),
    "Force_Edit.png": ("test_force", (86, 290, 558, 475)),
}

OPEN_ICON_CROP = (116, 80, 252, 174)
OPEN_TEXT_CROP = (394, 144, 484, 202)


def backup_files() -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup = ROOT / "Release" / f"before_window5_bitmap_reference_buttons_{stamp}"
    backup.mkdir(parents=True, exist_ok=False)
    for name in CROPS:
        src = RESOURCES / name
        if src.exists():
            shutil.copy2(src, backup / name)
    return backup


def antialiased_round_mask(size: tuple[int, int], radius: int) -> Image.Image:
    scale = 4
    mask = Image.new("L", (size[0] * scale, size[1] * scale), 0)
    draw = ImageDraw.Draw(mask)
    # Slight inset avoids leaving a square white fringe from the source screenshot corners.
    draw.rounded_rectangle(
        (1 * scale, 1 * scale, (size[0] - 2) * scale, (size[1] - 2) * scale),
        radius=radius * scale,
        fill=255,
    )
    return mask.resize(size, Image.Resampling.LANCZOS)


def normalize_blue_and_alpha(image: Image.Image) -> Image.Image:
    image = image.convert("RGBA")
    # Downsampled screenshot text can get a little soft; this keeps strokes crisp without stair-stepping.
    image = image.filter(ImageFilter.UnsharpMask(radius=0.45, percent=65, threshold=3))
    mask = antialiased_round_mask(image.size, radius=9)
    image.putalpha(mask)
    return image


def blue_foreground(crop: Image.Image, alpha_boost: int = 35) -> Image.Image:
    image = crop.convert("RGBA")
    r, g, b, _ = image.split()
    # Keep only the saturated blue strokes from the reference screenshot.
    mask = Image.eval(
        ImageChops.lighter(
            ImageChops.subtract(b, r),
            ImageChops.subtract(b, g),
        ),
        lambda px: max(0, min(255, (px - 18) * 4 + alpha_boost)),
    )
    mask = mask.filter(ImageFilter.GaussianBlur(0.35))
    image.putalpha(mask)
    return image


def local_button_fill(base: Image.Image) -> tuple[int, int, int, int]:
    # Use the button's own pale fill instead of a hard-coded white patch.
    sample = base.crop((54, 24, 66, 36)).convert("RGBA")
    stat = ImageStat.Stat(sample)
    return tuple(int(v) for v in stat.median[:3]) + (255,)


def clear_rect(base: Image.Image, box: tuple[int, int, int, int]) -> None:
    fill = local_button_fill(base)
    patch = Image.new("RGBA", (box[2] - box[0], box[3] - box[1]), fill)
    # Feather the clearing mask so the button gradient/shadow does not show a square seam.
    mask = Image.new("L", patch.size, 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle((0, 0, patch.size[0] - 1, patch.size[1] - 1), radius=8, fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(1.2))
    base.paste(patch, box[:2], mask)


def compose_open_from_close(sources: dict[str, Image.Image]) -> Image.Image:
    base = fit_crop(sources["close_sensor"].crop(CROPS["Close_Valve.png"][1]))
    clear_rect(base, (8, 8, 59, 50))
    clear_rect(base, (66, 10, 113, 48))

    icon = blue_foreground(sources["open_old"].crop(OPEN_ICON_CROP)).resize((48, 35), Image.Resampling.LANCZOS)
    text = blue_foreground(sources["open_old"].crop(OPEN_TEXT_CROP)).resize((40, 26), Image.Resampling.LANCZOS)
    base.alpha_composite(icon, (11, 12))
    base.alpha_composite(text, (72, 17))
    return normalize_blue_and_alpha(base)


def fit_crop(crop: Image.Image) -> Image.Image:
    image = crop.convert("RGBA")
    image = image.resize((120, 60), Image.Resampling.LANCZOS)
    return normalize_blue_and_alpha(image)


def render_buttons() -> None:
    missing = [str(path) for path in LATEST_REFS.values() if not path.exists()]
    if missing:
        raise FileNotFoundError("missing reference image(s): " + ", ".join(missing))

    sources = {name: Image.open(path).convert("RGBA") for name, path in LATEST_REFS.items()}
    compose_open_from_close(sources).save(RESOURCES / "Open_Valve.png", optimize=True)
    for output_name, (source_name, box) in CROPS.items():
        image = fit_crop(sources[source_name].crop(box))
        image.save(RESOURCES / output_name, optimize=True)


def main() -> None:
    backup = backup_files()
    render_buttons()
    print(f"applied bitmap reference Window5 buttons with antialiased rounded edges; backup={backup}")


if __name__ == "__main__":
    main()
