#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
BACKUP_DIR = ROOT / "Release" / f"before_remove_screenshot_buttons_{time.strftime('%Y%m%d_%H%M%S')}"

SCREENSHOT_CAPTIONS = {"GlobalScreenshotButton"}
SCREENSHOT_IDS = {20120}


def is_screenshot_control(value: object) -> bool:
    return (
        isinstance(value, dict)
        and (
            value.get("caption") in SCREENSHOT_CAPTIONS
            or value.get("id") in SCREENSHOT_IDS
        )
    )


def remove_screenshot_controls(node: object, removed: list[tuple[str, str, object]]) -> None:
    if isinstance(node, dict):
        for key, value in list(node.items()):
            if is_screenshot_control(value):
                removed.append((key, str(value.get("caption") or ""), value.get("id")))
                del node[key]
            else:
                remove_screenshot_controls(value, removed)
    elif isinstance(node, list):
        kept = []
        for index, item in enumerate(node):
            if is_screenshot_control(item):
                removed.append((f"[{index}]", str(item.get("caption") or ""), item.get("id")))
            else:
                remove_screenshot_controls(item, removed)
                kept.append(item)
        node[:] = kept


def count_screenshot_controls(node: object) -> int:
    if is_screenshot_control(node):
        return 1
    if isinstance(node, dict):
        return sum(count_screenshot_controls(value) for value in node.values())
    if isinstance(node, list):
        return sum(count_screenshot_controls(item) for item in node)
    return 0


def main() -> int:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=False)
    changed = []

    for ftu in sorted(UI_DIR.glob("*.ftu")):
        data, header, _ = decode_ftu(ftu)
        removed: list[tuple[str, str, object]] = []
        remove_screenshot_controls(data, removed)
        if not removed:
            continue

        shutil.copy2(ftu, BACKUP_DIR / "ui" / ftu.name)
        ftu.write_bytes(encode_ftu(data, header))

        decoded, _, _ = decode_ftu(ftu)
        remaining = count_screenshot_controls(decoded)
        if remaining:
            raise RuntimeError(f"{ftu.name}: {remaining} screenshot controls remain")

        changed.append((ftu.name, removed))

    for name, removed in changed:
        print(f"UPDATED {name}: removed {len(removed)} screenshot controls")
        for key, caption, control_id in removed:
            print(f"  {key} caption={caption} id={control_id}")
    print(f"changed_files={len(changed)}")
    print(f"backup={BACKUP_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
