#!/usr/bin/env python3
"""Match the Window2 address header color to the other white headers."""

from __future__ import annotations

import shutil
from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
BACKUP_PATH = ROOT / "ui" / "main.ftu.before-address-color-fix-20260716.bak"


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
    address_header = find_caption(find_caption(layout, "Window2"), "AddressTipSubItem")
    shutil.copy2(UI_PATH, BACKUP_PATH)
    address_header["colorTab"] = {"color0": 0xFFFFFF}
    UI_PATH.write_bytes(encode_ftu(layout, header))

    verified, _, _ = decode_ftu(UI_PATH)
    verified_header = find_caption(find_caption(verified, "Window2"), "AddressTipSubItem")
    if verified_header.get("colorTab", {}).get("color0") != 0xFFFFFF:
        shutil.copy2(BACKUP_PATH, UI_PATH)
        raise RuntimeError("AddressTipSubItem color verification failed")
    print("AddressTipSubItem color set to white")


if __name__ == "__main__":
    main()
