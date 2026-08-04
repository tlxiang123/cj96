#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import hashlib
import shutil
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu


ROOT = Path(__file__).resolve().parents[1]
CURRENT = ROOT / "ui" / "main.ftu"
CHECKPOINT = ROOT / "Release" / "ui_debug_fix_backup_20260721_201748" / "main.ftu"
SAFETY_BACKUP = ROOT / "Release" / "restore_to_debug_button_checkpoint_backup" / "main.ftu"


def md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


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
    if not CHECKPOINT.is_file():
        raise FileNotFoundError(CHECKPOINT)

    checkpoint_data, _, _ = decode_ftu(CHECKPOINT)
    if find_caption(checkpoint_data, "CycleWindow") is None:
        raise RuntimeError("checkpoint has no CycleWindow")

    SAFETY_BACKUP.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(CURRENT, SAFETY_BACKUP)
    shutil.copy2(CHECKPOINT, CURRENT)

    current_data, _, _ = decode_ftu(CURRENT)
    cycle = find_caption(current_data, "CycleWindow")
    left_region = find_caption(current_data, "CycleRegionLeftWindow")
    right_region = find_caption(current_data, "CycleRegionRightWindow")
    print(f"restored {CURRENT}")
    print(f"source {CHECKPOINT}")
    print(f"safety_backup {SAFETY_BACKUP}")
    print(f"md5 {md5(CURRENT)}")
    print(f"CycleWindow yes={cycle is not None}")
    print(f"CycleRegionLeftWindow yes={left_region is not None}")
    print(f"CycleRegionRightWindow yes={right_region is not None}")


if __name__ == "__main__":
    main()
