#!/usr/bin/env python3
from copy import deepcopy
from pathlib import Path
from ftu_style import decode_ftu, encode_ftu

root = Path(__file__).resolve().parents[1]
target = root / "ui" / "main.ftu"
source = root / "backups" / "window8_compact_ring_legend_20260716_182953" / "ui" / "main.ftu"


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


current, header, _ = decode_ftu(target)
backup, _, _ = decode_ftu(source)
window1 = find(current, "Window1")
window8 = deepcopy(find(backup, "Window8"))
window8["backgroundPic"] = "window8_panel.png"
window1["window__38"] = window8
for key, value in list(window1.items()):
    if isinstance(value, dict) and value.get("caption") == "Window8RingButtonSurface":
        del window1[key]
target.write_bytes(encode_ftu(current, header))
print("restored Window8 child from the ring dashboard version")
