#!/usr/bin/env python3
"""Make the Window2 synchronization subitem independently clickable."""

from __future__ import annotations

import shutil
from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
BACKUP_PATH = ROOT / "ui" / "main.ftu.before-sync-touch-fix-20260716.bak"


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
    layout, header, _ = decode_ftu(UI_PATH)
    status = find_caption(find_caption(layout, "Window2"), "StatusSubItem")

    shutil.copy2(UI_PATH, BACKUP_PATH)
    status["touchable"] = True
    UI_PATH.write_bytes(encode_ftu(layout, header))

    verified, _, _ = decode_ftu(UI_PATH)
    verified_status = find_caption(find_caption(verified, "Window2"), "StatusSubItem")
    if verified_status.get("touchable") is not True:
        shutil.copy2(BACKUP_PATH, UI_PATH)
        raise RuntimeError("StatusSubItem verification failed; restored backup")
    print("Window2 StatusSubItem is now touchable")


if __name__ == "__main__":
    main()
