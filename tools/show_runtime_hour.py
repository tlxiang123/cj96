#!/usr/bin/env python3
"""Show and align the hour, minute, and second runtime fields."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from ftu_style import decode_ftu, encode_ftu  # noqa: E402


FTU_PATH = ROOT / "ui" / "main.ftu"


def find_control(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def set_position(control: dict, left: int, top: int, width: int, height: int) -> None:
    control["position"] = {
        "height": height,
        "left": left,
        "top": top,
        "width": width,
    }


def main() -> None:
    data, header, _ = decode_ftu(FTU_PATH)
    window = find_control(data, "SetRunTimeWindow")
    title = find_control(window, "Button16")
    title["text"] = "当前阀组[x] 设置运行时间"
    set_position(title, 60, 18, 440, 42)

    fields = (
        ("HourEditText", 70),
        ("MinEditText", 245),
        ("SecEditText", 420),
    )
    labels = (
        ("Button17", 125),
        ("Button18", 300),
        ("Button19", 475),
    )
    for caption, left in fields:
        control = find_control(window, caption)
        control["visible"] = True
        set_position(control, left, 82, 50, 38)
    for caption, left in labels:
        control = find_control(window, caption)
        control["visible"] = True
        set_position(control, left, 82, 40, 38)

    for key, value in list(data.items()):
        if isinstance(value, dict) and value.get("caption") == "RunTimeScopeWindow":
            del data[key]

    button_style = {
        "alignment": 37,
        "colorTab": {"color0": 16777215, "color1": 16777215},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 20,
    }
    cancel_pics = {
        "pic0": "w2_group_bind_cancel_120x60.png",
        "pic1": "w2_group_bind_cancel_120x60.png",
        "pic2": "w2_group_bind_cancel_120x60.png",
    }
    current_pics = {
        "pic0": "w2_clear_current_120x60.png",
        "pic1": "w2_clear_current_120x60.png",
        "pic2": "w2_clear_current_120x60.png",
    }
    all_pics = {
        "pic0": "w2_clear_all_120x60.png",
        "pic1": "w2_clear_all_120x60.png",
        "pic2": "w2_clear_all_120x60.png",
    }
    data["window__266"] = {
        "backgroundPic": "w2_group_rename_dialog_560x220.png",
        "beepEnable": True,
        "caption": "RunTimeScopeWindow",
        "id": 110052,
        "visible": False,
        "position": {"height": 220, "left": 224, "top": 95, "width": 560},
        "textview__267": {
            "alignment": 37,
            "bold": True,
            "caption": "RunTimeScopePromptText",
            "colorTab": {"color0": 23483},
            "family": "Alibaba-PuHuiTi-Regular",
            "fontSize": 24,
            "id": 50094,
            "text": "编辑当前阀组的运行时间，\n还是编辑所有阀组？",
            "touchable": False,
            "position": {"height": 105, "left": 30, "top": 20, "width": 500},
        },
        "button__268": dict(button_style, caption="RunTimeScopeCancelButton", id=20189,
                            picTab=cancel_pics,
                            position={"height": 60, "left": 47, "top": 141, "width": 120}),
        "button__269": dict(button_style, caption="RunTimeScopeCurrentButton", id=20190,
                            picTab=current_pics,
                            position={"height": 60, "left": 196, "top": 141, "width": 120}),
        "button__270": dict(button_style, caption="RunTimeScopeAllButton", id=20191,
                            picTab=all_pics,
                            position={"height": 60, "left": 340, "top": 141, "width": 120}),
    }

    FTU_PATH.write_bytes(encode_ftu(data, header))

    verified, _, _ = decode_ftu(FTU_PATH)
    verified_window = find_control(verified, "SetRunTimeWindow")
    for caption, _ in fields + labels:
        if find_control(verified_window, caption).get("visible") is not True:
            raise RuntimeError(f"{caption} is not visible")
    scope = find_control(verified, "RunTimeScopeWindow")
    for caption in ("RunTimeScopeCancelButton", "RunTimeScopeCurrentButton", "RunTimeScopeAllButton"):
        find_control(scope, caption)
    print("SetRunTimeWindow now shows hour, minute, and second fields")


if __name__ == "__main__":
    main()
