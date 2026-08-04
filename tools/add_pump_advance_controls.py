#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

import ftu_style  # noqa: E402


MAIN_FTU = ROOT / "ui" / "main.ftu"
ADVANCE_EDIT_ID = 51043
ADVANCE_LABEL_ID = 50095
ADVANCE_UNIT_ID = 50096


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


def remove_ids(node: object, ids: set[int]) -> None:
    if isinstance(node, dict):
        for key, value in list(node.items()):
            if isinstance(value, dict) and value.get("id") in ids:
                del node[key]
            else:
                remove_ids(value, ids)
    elif isinstance(node, list):
        node[:] = [
            value for value in node
            if not (isinstance(value, dict) and value.get("id") in ids)
        ]
        for value in node:
            remove_ids(value, ids)


def text_node(caption: str, control_id: int, text: str, left: int, width: int) -> dict:
    return {
        "alignment": 37,
        "caption": caption,
        "colorTab": {"color0": 23483},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 20,
        "id": control_id,
        "text": text,
        "touchable": False,
        "position": {"height": 40, "left": left, "top": 256, "width": width},
    }


def main() -> None:
    data, header, _ = ftu_style.decode_ftu(MAIN_FTU)
    remove_ids(data, {ADVANCE_EDIT_ID, ADVANCE_LABEL_ID, ADVANCE_UNIT_ID})

    region = find_caption(data, "GroupBindRegion2Window")
    region["textview__pump_advance_label"] = text_node(
        "PumpAdvanceLabelText", ADVANCE_LABEL_ID, "提前", 34, 56)
    region["edittext__pump_advance_sec"] = {
        "alignment": 37,
        "caption": "PumpAdvanceSecEditText",
        "colorTab": {"color0": 23483},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 22,
        "id": ADVANCE_EDIT_ID,
        "text": "5",
        "position": {"height": 40, "left": 94, "top": 256, "width": 46},
        "backgroundPic": "cycle_edit_46x40.png",
    }
    region["textview__pump_advance_unit"] = text_node(
        "PumpAdvanceUnitText", ADVANCE_UNIT_ID, "秒", 148, 40)

    MAIN_FTU.write_bytes(ftu_style.encode_ftu(data, header))
    print("added PumpAdvanceSecEditText default 5 seconds")


if __name__ == "__main__":
    main()
