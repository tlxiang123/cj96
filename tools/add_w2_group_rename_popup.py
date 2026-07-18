#!/usr/bin/env python3
"""Add the two-row group rename popup without changing existing W2 layout positions."""

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


def main() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    w2 = find_caption(layout, "w2set_window")

    # The popup is a child of w2set_window, so it stays inside the existing W2
    # page and does not move any of the user-positioned region controls.
    for key, value in list(w2.items()):
        if isinstance(value, dict) and value.get("caption") == "GroupRenameWindow":
            del w2[key]

    used_keys = {key for key in w2 if key.startswith("window__")}
    key_number = 230
    while f"window__{key_number}" in used_keys:
        key_number += 1

    w2[f"window__{key_number}"] = {
        "backgroundPic": "w2_group_rename_dialog_560x220.png",
        "beepEnable": True,
        "caption": "GroupRenameWindow",
        "id": 110020,
        "visible": False,
        "position": {"height": 220, "left": 224, "top": 95, "width": 560},
        "edittext__231": {
            "alignment": 37,
            "backgroundPic": "w2_group_rename_edit_360x61.png",
            "beepEnable": True,
            "caption": "GroupNameEditText",
            "colorTab": {"color0": 23483},
            "family": "Alibaba-PuHuiTi-Regular",
            "fontSize": 28,
            "hintTextColor": 23483,
            "id": 51038,
            "position": {"height": 61, "left": 100, "top": 24, "width": 360},
            "touchable": True,
        },
        "button__232": {
            "alignment": 37,
            "caption": "GroupRenameOkButton",
            "id": 20179,
            "picTab": {
                "pic0": "w2_group_rename_confirm_120x60.png",
                "pic2": "w2_group_rename_confirm_120x60.png",
            },
            "position": {"height": 60, "left": 295, "top": 145, "width": 120},
        },
        "button__233": {
            "alignment": 37,
            "caption": "GroupRenameCancelButton",
            "id": 20180,
            "picTab": {
                "pic0": "w2_group_bind_cancel_120x60.png",
                "pic2": "w2_group_bind_cancel_120x60.png",
            },
            "position": {"height": 60, "left": 145, "top": 145, "width": 120},
        },
    }

    # Keep the header name blue while preserving its existing position.
    name_edit = find_caption(w2, "W2_NameEditText")
    name_edit["colorTab"] = {"color0": 23483}
    name_edit["hintTextColor"] = 23483

    # Restore the generated bindings for the W2 controls.  Earlier layout
    # edits shifted these IDs, which left the visible buttons without their
    # corresponding callbacks (and made GroupBind_Button collide with the
    # old W2 confirm button).
    canonical_ids = {
        "W2_OkButton": 20011,
        "W2_CencelButton": 20013,
        "GroupBind_Button": 20014,
        "ClearIrr_Button": 20006,
        "W2_DelButton": 20012,
        "GroupCencelButton": 20015,
        "GroupDelButton": 20017,
        "GroupAddButton": 20018,
        "SelectPumpButton": 20019,
        "SelectSenserButton": 20020,
        "Button24": 20061,
        "Button25": 20062,
        "Button29": 20064,
        "Button30": 20065,
        # Decorative Window3 icons are not event targets; keep their IDs
        # unique so they cannot shadow the W2 callbacks above.
        "Window3ProgramIcon": 50161,
        "Window3WeekModeIcon": 50162,
        "Window3IntervalIcon": 50164,
        "Window3DelayIcon": 50165,
    }
    for caption, control_id in canonical_ids.items():
        find_caption(layout, caption)["id"] = control_id

    UI_PATH.write_bytes(encode_ftu(layout, header))
    print(f"added GroupRenameWindow as window__{key_number}")


if __name__ == "__main__":
    main()
