#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
BACKUP_DIR = ROOT / "Release" / f"before_window5_source_target_blue_{time.strftime('%Y%m%d_%H%M%S')}"
BLUE = 23483


def find_caption(node: object, caption: str) -> dict | None:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            found = find_caption(value, caption)
            if found is not None:
                return found
    elif isinstance(node, list):
        for value in node:
            found = find_caption(value, caption)
            if found is not None:
                return found
    return None


def main() -> None:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    path = UI_DIR / "main.ftu"
    shutil.copy2(path, BACKUP_DIR / "ui" / "main.ftu")

    data, header, _ = decode_ftu(path)
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found")
    for caption in ("TextView12", "TextView17"):
        node = find_caption(window5, caption)
        if node is None:
            raise RuntimeError(f"{caption} not found")
        node["colorTab"] = {"color0": BLUE}

    path.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(path)
    for caption in ("TextView12", "TextView17"):
        node = find_caption(find_caption(decoded, "Window5"), caption)
        if node.get("colorTab", {}).get("color0") != BLUE:
            raise RuntimeError(f"{caption} color not blue")
    print(f"backup={BACKUP_DIR}")
    print("Window5 source/target labels set to blue")


if __name__ == "__main__":
    main()
