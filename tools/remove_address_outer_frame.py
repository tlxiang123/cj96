#!/usr/bin/env python3
"""Remove the joined outer frame while preserving address artwork."""

from pathlib import Path
import shutil

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
BACKUP_DIR = ROOT / "Release" / "address-frame-backup"


def window_background_crop(box: tuple[int, int, int, int]) -> Image.Image:
    background = Image.open(RESOURCE_DIR / "w2_window10_730x397.png").convert("RGB")
    return background.crop(box)


def remove_label_frame(path: Path) -> None:
    source = Image.open(path).convert("RGB")
    result = window_background_crop((25, 18, 138, 87))
    source_pixels = source.load()
    result_pixels = result.load()

    # Keep only the saturated blue location icon and label. The outer frame lies
    # outside this content area and is therefore replaced by the window background.
    for y in range(6, 64):
        for x in range(14, 100):
            red, green, blue = source_pixels[x, y]
            saturation = max(red, green, blue) - min(red, green, blue)
            if blue > red and blue > green and saturation > 22:
                alpha = min(1.0, (saturation - 18) / 54.0)
                base = result_pixels[x, y]
                result_pixels[x, y] = tuple(
                    round(base[channel] * (1.0 - alpha) + source_pixels[x, y][channel] * alpha)
                    for channel in range(3)
                )
    result.save(path, optimize=True)


def remove_value_frame(path: Path) -> None:
    source = Image.open(path).convert("RGB")
    result = window_background_crop((138, 18, 242, 87))

    # Preserve the three already aligned digit cards exactly. Their surrounding
    # joined rounded rectangle is supplied only by the source outside these crops.
    for left, right in ((4, 35), (35, 66), (65, 97)):
        card = source.crop((left, 9, right, 62))
        result.paste(card, (left, 9))
    result.save(path, optimize=True)


def main() -> None:
    label = RESOURCE_DIR / "w2_set_address_label_113x69.png"
    values = sorted(RESOURCE_DIR.glob("w2_set_address_value_*.png"))
    if len(values) != 255:
        raise RuntimeError(f"expected 255 address images, found {len(values)}")

    BACKUP_DIR.mkdir(parents=True, exist_ok=True)
    for path in [label, *values]:
        backup = BACKUP_DIR / path.name
        if not backup.exists():
            shutil.copy2(path, backup)

    remove_label_frame(label)
    for path in values:
        remove_value_frame(path)
    print("removed address outer frame from label and 255 value images")


if __name__ == "__main__":
    main()
