#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import hashlib
import shutil
import time
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu


ROOT = Path(__file__).resolve().parents[1]
CHECKPOINT = Path.home() / "Desktop" / "cj96_board_same_as_ide_20260721-191518" / "open_main_ftu_here"
BACKUP = ROOT / "Release" / f"before_restore_191518_{time.strftime('%Y%m%d_%H%M%S')}"


def md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def copy_tree_files(src: Path, dst: Path, pattern: str) -> list[str]:
    changed: list[str] = []
    for source in sorted(src.glob(pattern)):
        if not source.is_file():
            continue
        target = dst / source.name
        if not target.exists() or md5(source) != md5(target):
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, target)
            changed.append(str(target.relative_to(ROOT)))
    return changed


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
    if not (CHECKPOINT / "main.ftu").is_file():
        raise FileNotFoundError(CHECKPOINT / "main.ftu")

    backup_ui = BACKUP / "ui"
    backup_res = BACKUP / "resources"
    backup_ui.mkdir(parents=True, exist_ok=True)
    backup_res.mkdir(parents=True, exist_ok=True)
    for path in (ROOT / "ui").glob("*.ftu"):
        shutil.copy2(path, backup_ui / path.name)
    for path in (ROOT / "resources").glob("*"):
        if path.is_file():
            shutil.copy2(path, backup_res / path.name)

    changed: list[str] = []
    changed.extend(copy_tree_files(CHECKPOINT, ROOT / "ui", "*.ftu"))
    checkpoint_res = CHECKPOINT / "resources"
    if checkpoint_res.is_dir():
        changed.extend(copy_tree_files(checkpoint_res, ROOT / "resources", "*"))

    data, _, _ = decode_ftu(ROOT / "ui" / "page1topset.ftu")
    display = find_caption(data, "DisplayBtn")
    debug = find_caption(data, "DebugBtn")

    main_data, _, _ = decode_ftu(ROOT / "ui" / "main.ftu")
    cycle_left = find_caption(main_data, "CycleRegionLeftWindow")
    cycle_right = find_caption(main_data, "CycleRegionRightWindow")

    print(f"checkpoint={CHECKPOINT}")
    print(f"backup={BACKUP}")
    print("changed:")
    for item in changed:
        print(item)
    print(f"DisplayBtn={display.get('position') if display else None}")
    print(f"DebugBtn={debug.get('position') if debug else None}")
    print(f"CycleRegionLeftWindow={cycle_left is not None}")
    print(f"CycleRegionRightWindow={cycle_right is not None}")
    print(f"main_md5={md5(ROOT / 'ui' / 'main.ftu')}")


if __name__ == "__main__":
    main()
