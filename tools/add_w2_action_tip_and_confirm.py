#!/usr/bin/env python3
"""Restore the W2 no-selection notice and build the requested confirm button."""

from pathlib import Path

from PIL import Image

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
CONFIRM_SOURCE = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-a8377d13-cf67-4ee3-9e8e-0620196f3bb4.png")
CONFIRM_OUTPUT = ROOT / "resources" / "w2_group_rename_confirm_120x60.png"


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


def build_confirm_image() -> None:
    image = Image.open(CONFIRM_SOURCE).convert("RGB")
    # Keep the supplied rounded border and remove the outer screenshot margin.
    cropped = image.crop((76, 24, 450, 211))
    cropped.resize((120, 60), Image.Resampling.LANCZOS).save(
        CONFIRM_OUTPUT, format="PNG", optimize=True
    )


def add_action_tip_window() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    w2 = find_caption(layout, "w2set_window")

    for key, value in list(w2.items()):
        if isinstance(value, dict) and value.get("caption") == "W2ActionTipWindow":
            del w2[key]

    used = {key for key in w2 if key.startswith("window__")}
    number = 240
    while f"window__{number}" in used:
        number += 1

    w2[f"window__{number}"] = {
        "backgroundPic": "w2_tip_center_560x140.png",
        "beepEnable": True,
        "caption": "W2ActionTipWindow",
        "id": 110019,
        "visible": False,
        "position": {"height": 140, "left": 224, "top": 140, "width": 560},
        "textview__241": {
            "alignment": 37,
            "bold": True,
            "caption": "W2ActionTipTextView",
            "colorTab": {"color0": 23483},
            "fontSize": 30,
            "id": 50064,
            "text": "",
            "touchable": False,
            "position": {"height": 140, "left": 0, "top": 0, "width": 560},
        },
    }

    # Keep the confirmation image in the rename popup and preserve all popup
    # and W2 region coordinates already adjusted by the user.
    ok = find_caption(layout, "GroupRenameOkButton")
    ok["picTab"] = {
        "pic0": CONFIRM_OUTPUT.name,
        "pic2": CONFIRM_OUTPUT.name,
    }
    UI_PATH.write_bytes(encode_ftu(layout, header))
    print(f"added W2ActionTipWindow as window__{number} and updated confirm image")


if __name__ == "__main__":
    build_confirm_image()
    add_action_tip_window()
