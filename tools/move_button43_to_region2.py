#!/usr/bin/env python3
"""Move Button43 beside W2_OkButton without changing any other positions."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    raise LookupError(caption)


def pop_caption(node: object, caption: str) -> dict:
    if not isinstance(node, dict):
        raise LookupError(caption)
    for key, value in list(node.items()):
        if isinstance(value, dict) and value.get("caption") == caption:
            del node[key]
            return value
        if isinstance(value, dict):
            try:
                return pop_caption(value, caption)
            except LookupError:
                pass
    raise LookupError(caption)


layout, header, _ = decode_ftu(UI_PATH)
w2 = find_caption(layout, "w2set_window")
region2 = find_caption(w2, "W2SetRegion2Window")
ok = find_caption(region2, "W2_OkButton")
button = pop_caption(w2, "Button43")
button.pop("text", None)
button["picTab"] = {
    "pic0": "w2_one_click_add_120x60.png",
    "pic2": "w2_one_click_add_120x60.png",
}
button["position"] = {
    "height": 60,
    "left": ok["position"]["left"] + ok["position"]["width"] + 15,
    "top": ok["position"]["top"],
    "width": 120,
}
region2["button__278"] = button
UI_PATH.write_bytes(encode_ftu(layout, header))
print("moved Button43 to the right of W2_OkButton")
