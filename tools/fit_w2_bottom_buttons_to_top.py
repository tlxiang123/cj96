#!/usr/bin/env python3
"""Fit the bottom W2 button artwork into the top buttons' visual bounds."""

from pathlib import Path

from PIL import Image

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
UI_PATH = ROOT / "ui" / "main.ftu"
PANEL = Image.open(RESOURCES / "w2set_region_3.png").convert("RGB")

BUTTONS = {
    "W2_DelButton": ("w2_set_delete_group_110x113_v2.png", "w2_set_delete_group_113x113_norm.png", (20, 230)),
    "GroupNameButton": ("w2_set_rename_group_110x113_v3.png", "w2_set_rename_group_113x113_norm.png", (135, 230)),
}


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


def fit(source_name: str, output_name: str, left: int, top: int) -> None:
    base = PANEL.crop((left, top, left + 113, top + 113))
    source = Image.open(RESOURCES / source_name).convert("RGB").resize(
        (99, 102), Image.Resampling.LANCZOS
    )
    source_px = source.load()
    result_px = base.load()
    for y in range(102):
        for x in range(99):
            r, g, b = source_px[x, y]
            if min(r, g, b) < 220 or b - r > 25:
                result_px[x + 8, y + 3] = (r, g, b)
    result = base
    result.save(RESOURCES / output_name, format="PNG", optimize=True)


def main() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    for caption, (source, output, (left, top)) in BUTTONS.items():
        fit(source, output, left, top)
        find_caption(layout, caption)["picTab"] = {"pic0": output, "pic2": output}
    UI_PATH.write_bytes(encode_ftu(layout, header))
    print("fit bottom W2 buttons to the top button bounds")


if __name__ == "__main__":
    main()
