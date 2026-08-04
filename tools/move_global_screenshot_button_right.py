#!/usr/bin/env python3
from __future__ import annotations

import shutil
import sys
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
UI_DIR = ROOT / "ui"
BUTTON_CAPTION = "GlobalScreenshotButton"
BUTTON_ID = 20120
NEW_POSITION = {"left": 60, "top": 8, "width": 105, "height": 55}


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, (list, tuple)):
        for value in node:
            yield from walk(value)


def main() -> None:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup = ROOT / "Release" / f"before_move_global_screenshot_button_{stamp}"
    backup.mkdir(parents=True, exist_ok=False)

    changed = []
    for ftu_path in sorted(UI_DIR.glob("*.ftu")):
        data, header, _offset = decode_ftu(ftu_path)
        moved = False
        for node in walk(data):
            if node.get("caption") == BUTTON_CAPTION or node.get("id") == BUTTON_ID:
                if node.get("caption") == BUTTON_CAPTION:
                    node["position"] = dict(NEW_POSITION)
                    moved = True
        if moved:
            shutil.copy2(ftu_path, backup / ftu_path.name)
            ftu_path.write_bytes(encode_ftu(data, header))
            changed.append(ftu_path.name)

    if not changed:
        raise RuntimeError("no GlobalScreenshotButton found")

    print("moved GlobalScreenshotButton to", NEW_POSITION, "files=", ",".join(changed), "backup=", backup)


if __name__ == "__main__":
    main()
