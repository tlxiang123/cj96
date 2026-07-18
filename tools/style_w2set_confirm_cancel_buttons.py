#!/usr/bin/env python3
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


def style(control: dict, image: str, left: int) -> None:
    control.pop("text", None)
    control["picTab"] = {"pic0": image, "pic2": image}
    control["iconPosition"] = {"height": 60, "left": 0, "top": 0, "width": 120}
    control["position"] = {"height": 60, "left": left, "top": 215, "width": 120}


data, header, _ = decode_ftu(FTU)
region2 = find(data, "W2SetRegion2Window")
style(find(region2, "W2_OkButton"), "w2_ok_120x60.png", 145)
style(find(region2, "W2_CencelButton"), "w2_group_bind_cancel_120x60.png", 325)
FTU.write_bytes(encode_ftu(data, header))
print("styled W2 confirm and cancel buttons")
