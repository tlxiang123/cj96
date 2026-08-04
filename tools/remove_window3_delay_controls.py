#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import time

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
RELEASE = ROOT / "Release"
REMOVE_CAPTIONS = {
    "AfterDayEditText",
    "Window3DelayIcon",
    "W3AfterDayPickerButton",
}


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def remove_matching(node: object) -> int:
    removed = 0
    if isinstance(node, dict):
        for key in list(node.keys()):
            value = node[key]
            if isinstance(value, dict) and value.get("caption") in REMOVE_CAPTIONS:
                del node[key]
                removed += 1
            else:
                removed += remove_matching(value)
    elif isinstance(node, list):
        for value in node:
            removed += remove_matching(value)
    return removed


def main() -> int:
    data, header, _ = decode_ftu(MAIN_FTU)
    before = {
        item.get("caption")
        for item in walk(data)
        if isinstance(item, dict) and item.get("caption") in REMOVE_CAPTIONS
    }

    stamp = time.strftime("%Y%m%d_%H%M%S")
    backup_dir = RELEASE / f"window3_remove_delay_backup_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup_dir / "main.before.ftu")

    removed = remove_matching(data)
    MAIN_FTU.write_bytes(encode_ftu(data, header))
    shutil.copy2(MAIN_FTU, backup_dir / "main.after.ftu")
    print(f"removed={removed} captions={sorted(before)}")
    print(f"updated {MAIN_FTU}")
    print(f"backup {backup_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
