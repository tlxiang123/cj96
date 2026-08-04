#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import ftu_style  # noqa: E402


MAIN_FTU = ROOT / "ui" / "main.ftu"


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def find_caption(root: dict, caption: str) -> dict:
    for node in walk(root):
        if isinstance(node, dict) and node.get("caption") == caption:
            return node
    raise RuntimeError(f"missing control: {caption}")


def main() -> None:
    data, header, _ = ftu_style.decode_ftu(MAIN_FTU)
    listview = find_caption(data, "SelectPumpListView")
    item = find_caption(listview, "item")

    list_pos = listview.setdefault("position", {})
    item_pos = item.setdefault("position", {})

    old = {
        "cols": listview.get("cols"),
        "rows": listview.get("rows"),
        "list_height": list_pos.get("height"),
        "item_width": item_pos.get("width"),
        "item_height": item_pos.get("height"),
        "icon_left": item.get("iconPosition", {}).get("left"),
    }

    listview["cols"] = 1
    listview["rows"] = 2
    listview["colSpacing"] = 0
    listview["rowSpacing"] = 5
    list_pos["height"] = 109
    item_pos["width"] = list_pos.get("width", 225)
    item_pos["height"] = 52
    icon_pos = item.setdefault("iconPosition", {})
    icon_pos["left"] = item_pos["width"] - icon_pos.get("width", 28) - 18

    MAIN_FTU.write_bytes(ftu_style.encode_ftu(data, header))
    print(f"SelectPumpListView preview changed from {old} to "
          f"cols={listview['cols']}, rows={listview['rows']}, "
          f"list_height={list_pos['height']}, item_width={item_pos['width']}, "
          f"item_height={item_pos['height']}, icon_left={icon_pos['left']}")


if __name__ == "__main__":
    main()
