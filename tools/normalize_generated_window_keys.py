#!/usr/bin/env python3
"""Use numeric FTU widget keys for generated Window nodes."""

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
w2set = find(data, "w2set_window")
for old, new in (
    ("window__w2_region1", "window__217"),
    ("window__w2_region2", "window__218"),
    ("window__w2_region3", "window__219"),
):
    if old in w2set:
        w2set[new] = w2set.pop(old)
if "window__window8_root" in data:
    data["window__220"] = data.pop("window__window8_root")
FTU.write_bytes(encode_ftu(data, header))
print("normalized generated Window keys")
