#!/usr/bin/env python3
"""Split group rename into a clean icon button and a separate edit dialog."""

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
UI_PATH = ROOT / "ui" / "main.ftu"
DIALOG_PIC = RESOURCE_DIR / "w2_group_rename_dialog_560x220.png"
EDIT_PIC = RESOURCE_DIR / "w2_group_rename_edit_360x61.png"


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


def find_parent(node: object, child: dict) -> dict:
    if isinstance(node, dict):
        if any(value is child for value in node.values()):
            return node
        for value in node.values():
            try:
                return find_parent(value, child)
            except LookupError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_parent(value, child)
            except LookupError:
                pass
    raise LookupError("parent")


def rounded_panel(path: Path, width: int, height: int, radius: int, border: int) -> None:
    scale = 4
    image = Image.new("RGB", (width * scale, height * scale), (250, 254, 255))
    pixels = image.load()
    for y in range(height * scale):
        ratio = y / max(1, height * scale - 1)
        color = (round(254 - 8 * ratio), round(255 - 2 * ratio), 255)
        for x in range(width * scale):
            pixels[x, y] = color
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (3 * scale, 3 * scale, (width - 4) * scale, (height - 4) * scale),
        radius=radius * scale,
        outline=(14, 151, 232),
        width=border * scale,
    )
    image.resize((width, height), Image.Resampling.LANCZOS).save(path, optimize=True)


def main() -> None:
    rounded_panel(DIALOG_PIC, 560, 220, 16, 3)
    rounded_panel(EDIT_PIC, 360, 61, 8, 2)

    layout, header, _ = decode_ftu(UI_PATH)
    old_edit = find_caption(layout, "GroupNameEditText")
    button_parent = find_parent(layout, old_edit)
    old_key = next(key for key, value in button_parent.items() if value is old_edit)
    old_position = dict(old_edit["position"])
    del button_parent[old_key]
    button_parent["button__179"] = {
        "alignment": 37,
        "beepEnable": True,
        "caption": "GroupNameButton",
        "id": 20078,
        "picTab": {
            "pic0": "w2_set_rename_group_110x113_v3.png",
            "pic1": "w2_set_rename_group_110x113_v3.png",
        },
        "position": old_position,
    }

    w2_set_window = find_caption(layout, "w2set_window")
    try:
        old_dialog = find_caption(w2_set_window, "GroupRenameWindow")
        dialog_key = next(key for key, value in w2_set_window.items() if value is old_dialog)
        del w2_set_window[dialog_key]
    except LookupError:
        pass
    w2_set_window["window__215"] = {
        "backgroundPic": DIALOG_PIC.name,
        "beepEnable": True,
        "caption": "GroupRenameWindow",
        "id": 110020,
        "visible": False,
        "position": {"height": 220, "left": 224, "top": 95, "width": 560},
        "edittext__216": {
            "alignment": 37,
            "backgroundPic": EDIT_PIC.name,
            "beepEnable": True,
            "caption": "GroupNameEditText",
            "colorTab": {"color0": 23483},
            "family": "Alibaba-PuHuiTi-Regular",
            "fontSize": 28,
            "id": 51038,
            "position": {"height": 61, "left": 100, "top": 30, "width": 360},
        },
        "button__217": {
            "alignment": 37,
            "caption": "GroupRenameOkButton",
            "id": 20079,
            "picTab": {"pic0": "w2_ok_120x60.png", "pic1": "w2_ok_120x60.png"},
            "position": {"height": 60, "left": 145, "top": 125, "width": 120},
        },
        "button__218": {
            "alignment": 37,
            "caption": "GroupRenameCancelButton",
            "id": 20080,
            "picTab": {"pic0": "w2_cancel_120x60.png", "pic1": "w2_cancel_120x60.png"},
            "position": {"height": 60, "left": 295, "top": 125, "width": 120},
        },
    }
    UI_PATH.write_bytes(encode_ftu(layout, header))
    print("built separate group rename button and dialog")


if __name__ == "__main__":
    main()
