#!/usr/bin/env python3
"""Match W2SetRegion3 button outside pixels to the region background."""

from pathlib import Path

from PIL import Image

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
UI_PATH = ROOT / "ui" / "main.ftu"
PANEL = Image.open(RESOURCES / "w2set_region_3.png").convert("RGB")

BUTTONS = {
    "GroupBind_Button": ("w2_set_bind_113x113.png", "w2_set_bind_113x113_norm.png", (13, 94)),
    "ClearIrr_Button": ("w2_set_clear_113x113.png", "w2_set_clear_113x113_norm.png", (135, 94)),
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


def align_button(source_name: str, output_name: str, left: int, top: int) -> None:
    source = Image.open(RESOURCES / source_name).convert("RGB").resize((113, 113), Image.Resampling.LANCZOS)
    background = PANEL.crop((left, top, left + 113, top + 113))
    result = background.copy()
    src_px = source.load()
    dst_px = result.load()
    for y in range(113):
        for x in range(113):
            r, g, b = src_px[x, y]
            # Preserve blue ink and anti-aliased edge pixels; replace only
            # the pale outer field with the exact region-3 background.
            if min(r, g, b) < 220 or b - r > 25:
                dst_px[x, y] = (r, g, b)
    result.save(RESOURCES / output_name, format="PNG", optimize=True)


def main() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    for caption, (source, output, (left, top)) in BUTTONS.items():
        align_button(source, output, left, top)
        button = find_caption(layout, caption)
        button["picTab"] = {"pic0": output, "pic2": output}
    UI_PATH.write_bytes(encode_ftu(layout, header))
    print("aligned W2SetRegion3 button backgrounds")


if __name__ == "__main__":
    main()
