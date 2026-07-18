#!/usr/bin/env python3
"""Arrange the four W2 set image buttons in a two-by-two grid."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU = ROOT / "ui" / "main.ftu"


def find(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def image_button(control: dict, image: str, width: int, height: int, left: int, top: int) -> None:
    control.pop("text", None)
    control["picTab"] = {"pic0": image, "pic2": image}
    control["iconPosition"] = {"height": height, "left": 0, "top": 0, "width": width}
    control["position"] = {"height": height, "left": left, "top": top, "width": width}


data, header, _ = decode_ftu(FTU)
region3 = find(data, "W2SetRegion3Window")
clear = find(region3, "ClearIrr_Button")
bind = find(region3, "GroupBind_Button")
delete = find(region3, "W2_DelButton")
image_button(bind, "w2_set_bind_113x113.png", 113, 113, 20, 48)
image_button(clear, "w2_set_clear_113x113.png", 113, 113, 145, 48)
image_button(delete, "w2_set_delete_group_110x113_v2.png", 110, 113, 20, 195)

rename = {
    "alignment": 37,
    "beepEnable": True,
    "caption": "GroupNameButton",
    "id": 20078,
    "picTab": {"pic0": "w2_set_rename_group_110x113_v3.png", "pic2": "w2_set_rename_group_110x113_v3.png"},
    "iconPosition": {"height": 113, "left": 0, "top": 0, "width": 110},
    "position": {"height": 113, "left": 165, "top": 195, "width": 110},
}
region3["button__202"] = rename

# Keep the irrigation number visible above the two-row action grid.
irr_num = find(region3, "IrrNum_TextView")
irr_num["position"] = {"height": 38, "left": 128, "top": 6, "width": 126}

FTU.write_bytes(encode_ftu(data, header))
print("arranged W2 image buttons in two rows and two columns")
