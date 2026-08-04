#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import time
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
RELEASE = ROOT / "Release"


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


def main() -> None:
    data, header, _ = decode_ftu(MAIN_FTU)
    window2 = find_caption(data, "Window2")

    tip_key = None
    tip_value = None
    for key, value in list(window2.items()):
        if isinstance(value, dict) and value.get("caption") == "W2TipWindow":
            tip_key = key
            tip_value = value
            break
    if tip_key is None or tip_value is None:
        raise RuntimeError("W2TipWindow missing")

    stamp = time.strftime("%Y%m%d_%H%M%S")
    backup_dir = RELEASE / f"w2_tip_front_backup_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup_dir / "main.before.ftu")

    del window2[tip_key]
    window2[tip_key] = tip_value

    MAIN_FTU.write_bytes(encode_ftu(data, header))
    shutil.copy2(MAIN_FTU, backup_dir / "main.after.ftu")

    verified, _, _ = decode_ftu(MAIN_FTU)
    verified_window2 = find_caption(verified, "Window2")
    last_child = None
    for value in verified_window2.values():
        if isinstance(value, dict) and value.get("caption"):
            last_child = value.get("caption")
    if last_child != "W2TipWindow":
        raise RuntimeError(f"W2TipWindow is not last child, last={last_child}")
    print(f"moved W2TipWindow to front backup_dir={backup_dir}")


if __name__ == "__main__":
    main()
