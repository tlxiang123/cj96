#!/usr/bin/env python3
"""Make w2set visual region windows pass touch events to their children."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "main.ftu"


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


data, header, _ = decode_ftu(FTU_PATH)
w2set = find(data, "w2set_window")
for caption in ("W2SetRegion1Window", "W2SetRegion2Window", "W2SetRegion3Window"):
    region = find(w2set, caption)
    region["touchable"] = False
FTU_PATH.write_bytes(encode_ftu(data, header))
print("w2set regions set to non-touchable visual containers")
