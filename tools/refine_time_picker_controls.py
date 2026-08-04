#!/usr/bin/env python3
"""Tighten AM/PM placement and match the standard dialog button assets."""

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


def style_image_button(control: dict, image: str, position: dict) -> None:
    for key in ("backgroundPic", "bgColorTab", "colorTab", "family", "fontSize", "text", "textPosition"):
        control.pop(key, None)
    control["alignment"] = 37
    control["picTab"] = {"pic0": image, "pic2": image}
    control["position"] = position


def main() -> None:
    data, header, _ = decode_ftu(UI_PATH)
    clock = find_caption(data, "DigitalClock1")
    ampm = find_caption(data, "AmPmText")
    clock["position"] = {"height": 44, "left": 110, "top": 362, "width": 116}
    ampm["position"] = {"height": 44, "left": 230, "top": 362, "width": 58}

    style_image_button(
        find_caption(data, "TimeCancelButton"),
        "w2_group_bind_cancel_120x60.png",
        {"height": 60, "left": 125, "top": 184, "width": 120},
    )
    style_image_button(
        find_caption(data, "TimeDoneButton"),
        "w2_group_rename_confirm_120x60.png",
        {"height": 60, "left": 275, "top": 184, "width": 120},
    )

    UI_PATH.write_bytes(encode_ftu(data, header))
    print(f"updated {UI_PATH}")


if __name__ == "__main__":
    main()
