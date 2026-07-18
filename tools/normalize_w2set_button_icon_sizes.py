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


data, header, _ = decode_ftu(FTU)
region3 = find(data, "W2SetRegion3Window")

delete = find(region3, "W2_DelButton")
delete["position"]["width"] = 113
delete["iconPosition"] = {"height": 107, "left": 7, "top": 3, "width": 99}

rename = find(region3, "GroupNameButton")
rename["position"]["width"] = 113
rename["iconPosition"] = {"height": 107, "left": 7, "top": 3, "width": 99}

FTU.write_bytes(encode_ftu(data, header))
print("normalized W2 delete icon and reduced rename icon")
