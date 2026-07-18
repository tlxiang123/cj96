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
region1 = find(data, "W2SetRegion1Window")
find(region1, "TextView3")["position"] = {"height": 48, "left": 335, "top": 20, "width": 95}
find(region1, "W2_NameEditText")["position"] = {"height": 41, "left": 440, "top": 22, "width": 140}
FTU.write_bytes(encode_ftu(data, header))
print("fit W2 region1 name controls inside 590px region")
