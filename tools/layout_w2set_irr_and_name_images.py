#!/usr/bin/env python3
"""Apply image-backed irrigation value and device-name controls."""

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


data, header, _ = decode_ftu(FTU)
region1 = find(data, "W2SetRegion1Window")
region3 = find(data, "W2SetRegion3Window")

name_label = find(region1, "TextView3")
name_label.pop("text", None)
name_label["backgroundPic"] = "w2_set_device_name_label_77x77.png"
name_label["position"] = {"height": 77, "left": 340, "top": 5, "width": 77}

name_edit = find(region1, "W2_NameEditText")
name_edit["colorTab"] = {"color0": 0x005BBB}
name_edit["hintTextColor"] = 0x005BBB
name_edit["position"] = {"height": 41, "left": 430, "top": 23, "width": 150}

irr_label = find(region3, "IrrNum_TextView")
irr_label.pop("text", None)
irr_label["backgroundPic"] = "w2_set_irr_label_113x69_v2.png"
irr_label["position"] = {"height": 69, "left": 82, "top": 10, "width": 113}

for key, value in list(region3.items()):
    if isinstance(value, dict) and value.get("caption") == "IrrNumValue_TextView":
        del region3[key]
region3["textview__203"] = {
    "alignment": 37,
    "caption": "IrrNumValue_TextView",
    "id": 50003,
    "backgroundPic": "w2_set_irr_value_001.png",
    "touchable": False,
    "position": {"height": 69, "left": 195, "top": 10, "width": 95},
}

find(region3, "GroupBind_Button")["position"] = {"height": 113, "left": 20, "top": 94, "width": 113}
find(region3, "ClearIrr_Button")["position"] = {"height": 113, "left": 145, "top": 94, "width": 113}
find(region3, "W2_DelButton")["position"] = {"height": 113, "left": 20, "top": 230, "width": 110}
find(region3, "GroupNameButton")["position"] = {"height": 113, "left": 165, "top": 230, "width": 110}

FTU.write_bytes(encode_ftu(data, header))
print("applied W2 irrigation and device-name image layout")
