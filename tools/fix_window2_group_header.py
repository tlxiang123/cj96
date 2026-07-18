#!/usr/bin/env python3
"""Restore the Window2 group header to its original column bounds."""

from __future__ import annotations

import shutil
from copy import deepcopy
from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
SOURCE_PATH = ROOT / "ui" / "main.ftu.before-group-header-fix-20260716.bak"
BACKUP_PATH = ROOT / "ui" / "main.ftu.before-group-header-restore-20260716.bak"
EXPECTED_POSITION = {"height": 55, "left": 615, "top": 0, "width": 142}


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
    window2 = find_caption(layout, "Window2")
    try:
        group_header = find_caption(window2, "ArreTipSubItem")
    except LookupError:
        source_layout, _, _ = decode_ftu(SOURCE_PATH)
        source_header = find_caption(find_caption(source_layout, "Window2"), "ArreTipSubItem")
        group_header = deepcopy(source_header)
        header_list = find_caption(window2, "DeviceTipListView")["item"]["subItem"]
        insert_at = next(
            (
                index
                for index, item in enumerate(header_list)
                if item.get("position", {}).get("left", 0) > EXPECTED_POSITION["left"]
            ),
            len(header_list),
        )
        header_list.insert(insert_at, group_header)
    shutil.copy2(UI_PATH, BACKUP_PATH)
    group_header["position"] = dict(EXPECTED_POSITION)
    UI_PATH.write_bytes(encode_ftu(layout, header))

    verified, _, _ = decode_ftu(UI_PATH)
    verified_header = find_caption(find_caption(verified, "Window2"), "ArreTipSubItem")
    if verified_header.get("position") != EXPECTED_POSITION:
        shutil.copy2(BACKUP_PATH, UI_PATH)
        raise RuntimeError("ArreTipSubItem position verification failed")
    print("ArreTipSubItem restored to 142x55 at x=615")


if __name__ == "__main__":
    main()
