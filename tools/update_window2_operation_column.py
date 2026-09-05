#!/usr/bin/env python3
"""Add the Window2 operation column to the current IDE-saved main.ftu."""

from __future__ import annotations

import copy
import shutil
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
UI_BACKGROUND = ROOT / "ui" / "w2_bgr.png"
RESOURCE_BACKGROUND = ROOT / "resources" / "w2_bgr.png"

OPERATION_ITEM_ID = 24033
OPERATION_TIP_ID = 24034

# Positions are relative to the 976-pixel list item.
COLUMNS = {
    "AddressSubItem": (0, 145),
    "NameSubItem": (145, 190),
    "TypeSubItem": (335, 155),
    "ArreSubItem": (490, 190),
    "StatusSubItem": (680, 140),
    "OperationSubItem": (820, 156),
    "AddressTipSubItem": (0, 145),
    "NameTipSubItem": (145, 190),
    "TypeTipSubItem": (335, 155),
    "ArreTipSubItem": (490, 190),
    "StatusTipSubItem": (680, 140),
    "OperationTipSubItem": (820, 156),
}


def find_control(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_control(value, caption)
            except LookupError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_control(value, caption)
            except LookupError:
                pass
    raise LookupError(caption)


def set_position(item: dict, caption: str) -> None:
    left, width = COLUMNS[caption]
    item["position"]["left"] = left
    item["position"]["width"] = width


def add_operation_subitems(data: dict) -> None:
    value_list = find_control(data, "DeviceListView")
    tip_list = find_control(data, "DeviceTipListView")

    value_items = value_list["item"]["subItem"]
    tip_items = tip_list["item"]["subItem"]

    for item in value_items:
        set_position(item, item["caption"])
    for item in tip_items:
        set_position(item, item["caption"])

    value_items[:] = [item for item in value_items if item.get("id") != OPERATION_ITEM_ID]
    tip_items[:] = [item for item in tip_items if item.get("id") != OPERATION_TIP_ID]

    operation_item = copy.deepcopy(find_control(data, "StatusSubItem"))
    operation_item.update({
        "caption": "OperationSubItem",
        "id": OPERATION_ITEM_ID,
        "text": "删除",
        "touchable": True,
    })
    operation_item.pop("picTab", None)
    operation_item.pop("backgroundPic", None)
    set_position(operation_item, "OperationSubItem")
    value_items.append(operation_item)

    operation_tip = copy.deepcopy(find_control(data, "StatusTipSubItem"))
    operation_tip.update({
        "caption": "OperationTipSubItem",
        "id": OPERATION_TIP_ID,
        "text": "操作",
        "touchable": False,
    })
    operation_tip.pop("picTab", None)
    set_position(operation_tip, "OperationTipSubItem")
    tip_items.append(operation_tip)


def draw_background(path: Path) -> None:
    width, height = 1007, 400
    image = Image.new("RGB", (width, height), "white")
    pixels = image.load()
    for y in range(20, 369):
        ratio = (y - 20) / 348.0
        shade = (
            int(249 - 12 * ratio),
            int(253 - 7 * ratio),
            int(255 - 2 * ratio),
        )
        for x in range(19, 1000):
            pixels[x, y] = shade

    draw = ImageDraw.Draw(image)
    border = (25, 25, 25)
    draw.rounded_rectangle((19, 20, 999, 368), radius=13, outline=border, width=1)
    for y in (72, 132, 191, 250, 309):
        draw.line((19, y, 999, y), fill=border, width=1)
    for x in (164, 354, 509, 699, 839):
        draw.line((x, 20, x, 368), fill=border, width=1)
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path)


def main() -> None:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup = ROOT / "backups" / f"window2_operation_column_{stamp}"
    backup.mkdir(parents=True)
    shutil.copy2(MAIN_FTU, backup / "main.ftu")
    for source in (UI_BACKGROUND, RESOURCE_BACKGROUND):
        if source.exists():
            destination = backup / source.relative_to(ROOT)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)

    data, header, _ = decode_ftu(MAIN_FTU)
    add_operation_subitems(data)
    MAIN_FTU.write_bytes(encode_ftu(data, header))
    draw_background(RESOURCE_BACKGROUND)
    shutil.copy2(RESOURCE_BACKGROUND, UI_BACKGROUND)

    verify, _, _ = decode_ftu(MAIN_FTU)
    value_ids = [item.get("id") for item in find_control(verify, "DeviceListView")["item"]["subItem"]]
    tip_ids = [item.get("id") for item in find_control(verify, "DeviceTipListView")["item"]["subItem"]]
    if value_ids.count(OPERATION_ITEM_ID) != 1 or tip_ids.count(OPERATION_TIP_ID) != 1:
        raise RuntimeError("operation subitem verification failed")
    print(f"backup={backup}")
    print(f"DeviceListView subitems={value_ids}")
    print(f"DeviceTipListView subitems={tip_ids}")


if __name__ == "__main__":
    main()
