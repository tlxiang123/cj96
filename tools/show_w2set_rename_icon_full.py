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
rename = find(data, "GroupNameButton")
rename["picTab"] = {"pic0": "w2_set_rename_group_110x113_v3.png", "pic2": "w2_set_rename_group_110x113_v3.png"}
rename["iconPosition"] = {"height": 113, "left": 1, "top": 0, "width": 110}
rename["position"] = {"height": 113, "left": 165, "top": 230, "width": 113}
FTU.write_bytes(encode_ftu(data, header))
print("showed full-size W2 rename icon")
