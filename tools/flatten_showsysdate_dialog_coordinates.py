#!/usr/bin/env python3
"""Use root-coordinate dialog children so FlyThingsIDE matches runtime layout."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "showsysdate.ftu"


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def flatten_dialog(dialog: dict, panel_caption: str, panel_id: int) -> None:
    old_position = dict(dialog["position"])
    offset_x = old_position["left"]
    offset_y = old_position["top"]
    panel_image = dialog.pop("backgroundPic")

    controls = []
    for key, value in list(dialog.items()):
        if isinstance(value, dict) and value.get("caption") and "position" in value:
            controls.append((key, value))
            del dialog[key]

    dialog["position"] = {"height": 600, "left": 0, "top": 0, "width": 1024}
    dialog["window__panel"] = {
        "backgroundPic": panel_image,
        "beepEnable": False,
        "caption": panel_caption,
        "id": panel_id,
        "position": old_position,
        "touchable": False,
    }

    for key, control in controls:
        position = dict(control["position"])
        position["left"] += offset_x
        position["top"] += offset_y
        control["position"] = position
        dialog[key] = control


def main() -> None:
    data, header, _ = decode_ftu(UI_PATH)
    flatten_dialog(find_caption(data, "DatePickerWindow"), "DatePickerPanel", 110021)
    flatten_dialog(find_caption(data, "TimePickerWindow"), "TimePickerPanel", 110022)
    flatten_dialog(find_caption(data, "SyncFailureWindow"), "SyncFailurePanel", 110023)
    UI_PATH.write_bytes(encode_ftu(data, header))
    print(f"updated {UI_PATH}")


if __name__ == "__main__":
    main()
