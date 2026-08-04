#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import time

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
RELEASE = ROOT / "Release"


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def find_caption(node: object, caption: str) -> dict:
    for item in walk(node):
        if isinstance(item, dict) and item.get("caption") == caption:
            return item
    raise LookupError(caption)


def set_position(node: dict, left: int, top: int, width: int, height: int) -> None:
    node["position"] = {"height": height, "left": left, "top": top, "width": width}


def main() -> int:
    data, header, _ = decode_ftu(MAIN_FTU)
    region = find_caption(data, "Window3Region6Window")
    interval_icon = find_caption(region, "Window3IntervalIcon")
    interval_edit = find_caption(region, "IntervalDayEditText")

    stamp = time.strftime("%Y%m%d_%H%M%S")
    backup_dir = RELEASE / f"window3_interval_center_backup_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup_dir / "main.before.ftu")

    set_position(interval_icon, 132, 125, 96, 96)
    set_position(interval_edit, 258, 137, 99, 71)
    interval_edit["backgroundPic"] = "debug_edit_inner_99x71.png"
    interval_edit.pop("bgColorTab", None)
    interval_edit["fontSize"] = 34
    interval_edit["alignment"] = 37

    MAIN_FTU.write_bytes(encode_ftu(data, header))
    shutil.copy2(MAIN_FTU, backup_dir / "main.after.ftu")
    print(f"updated {MAIN_FTU}")
    print(f"backup {backup_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
