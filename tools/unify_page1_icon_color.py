from __future__ import annotations

import shutil
from datetime import datetime
from pathlib import Path

import numpy as np
from PIL import Image


WORKSPACE = Path(__file__).resolve().parents[1]
RESOURCE_DIR = WORKSPACE / "resources"
TARGET_COLOR = (21, 119, 181)

ICON_NAMES = (
    "topset_time_113.png",
    "topset_settings_header_clean_158x108.png",
    "topset_wifi_113.png",
    "topset_ethernet_113.png",
    "topset_4g_113.png",
    "topset_display_113.png",
    "topset_language_113.png",
    "topset_debug_113.png",
    "topset_remote_upgrade_113.png",
)


def recolor(path: Path) -> None:
    image = Image.open(path).convert("RGBA")
    pixels = np.asarray(image).copy()
    visible = pixels[:, :, 3] > 0
    pixels[visible, 0] = TARGET_COLOR[0]
    pixels[visible, 1] = TARGET_COLOR[1]
    pixels[visible, 2] = TARGET_COLOR[2]
    Image.fromarray(pixels, "RGBA").save(path)


def main() -> None:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_dir = WORKSPACE / "backups" / f"page1_icons_before_color_unify_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=True)

    for name in ICON_NAMES:
        path = RESOURCE_DIR / name
        if not path.exists():
            raise FileNotFoundError(path)
        shutil.copy2(path, backup_dir / name)

    for name in ICON_NAMES:
        recolor(RESOURCE_DIR / name)

    print(f"target_color=RGB{TARGET_COLOR}")
    print(f"backup={backup_dir}")
    print(f"resources={RESOURCE_DIR}")


if __name__ == "__main__":
    main()
