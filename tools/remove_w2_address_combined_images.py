#!/usr/bin/env python3
"""Switch Window2 address display from combined PNGs to live text and remove active combined images."""

from __future__ import annotations

import shutil
from datetime import datetime
from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
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


def update_ftu() -> None:
    layout, header, _ = decode_ftu(UI_PATH)

    label = find_caption(layout, "TextView1")
    label.pop("backgroundPic", None)
    label.pop("bgColorTab", None)
    label["text"] = "设备地址"
    label["position"] = {"height": 24, "left": 56, "top": 53, "width": 84}
    label["colorTab"] = {"color0": BLUE}
    label["fontSize"] = 20
    label["alignment"] = 37

    address = find_caption(layout, "W2_AddressEditText")
    address["visible"] = True
    address["text"] = "001"
    address["position"] = {"height": 46, "left": 150, "top": 17, "width": 108}
    address["colorTab"] = {"color0": BLUE}
    address["hintTextColor"] = BLUE
    address["fontSize"] = 32
    address["alignment"] = 37
    address.pop("bgColorTab", None)

    UI_PATH.write_bytes(encode_ftu(layout, header))


def remove_active_images() -> tuple[Path, int]:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup_root = ROOT / "backups" / f"w2_address_combined_removed_{stamp}"
    count = 0
    for folder_name in ["resources", "ui"]:
        folder = ROOT / folder_name
        for path in sorted(folder.glob("w2_set_address_combined_*.png")):
            backup_path = backup_root / folder_name / path.name
            backup_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(str(path), str(backup_path))
            count += 1
    return backup_root, count


def main() -> None:
    update_ftu()
    backup_root, count = remove_active_images()
    print("updated ui/main.ftu to use text address display")
    print(f"removed active combined images: {count}")
    print(f"backup: {backup_root}")


if __name__ == "__main__":
    main()
