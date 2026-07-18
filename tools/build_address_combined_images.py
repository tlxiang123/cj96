#!/usr/bin/env python3
"""Combine the fixed address label and dynamic digits into one reliable control image."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"


def main() -> None:
    label = Image.open(RESOURCE_DIR / "w2_set_address_label_77x77.png").convert("RGB")
    window = Image.open(RESOURCE_DIR / "w2_window10_730x397.png").convert("RGB")
    for address in range(1, 256):
        result = window.crop((43, 18, 242, 95))
        value = Image.open(
            RESOURCE_DIR / f"w2_set_address_value_{address:03d}.png"
        ).convert("RGB")
        result.paste(label, (0, 0))
        result.paste(value, (95, 0))
        result.save(
            RESOURCE_DIR / f"w2_set_address_combined_{address:03d}.png",
            optimize=True,
        )
    print("created 255 combined address images")


if __name__ == "__main__":
    main()
