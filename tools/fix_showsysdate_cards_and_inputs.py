#!/usr/bin/env python3
"""Fix system-time cards and add numeric date/time input dialogs."""

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "showsysdate.ftu"
RESOURCE_DIR = ROOT / "resources"
CARDS_NAME = "time_settings_left_panel_367x310.png"
PANEL = 0xF7FBFF
PRESSED = 0xEAF6FF
BLUE = 0x168BFF
BLUE_DARK = 0x005BBB
TEXT = 0x111318
WHITE = 0xFFFFFF


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


def pop_caption(parent: dict, caption: str) -> dict:
    for key, value in list(parent.items()):
        if isinstance(value, dict) and value.get("caption") == caption:
            del parent[key]
            return value
    raise KeyError(caption)


def build_cards() -> None:
    scale = 4
    size = (367, 310)
    image = Image.new("RGBA", (size[0] * scale, size[1] * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    for top in (2, 162):
        draw.rounded_rectangle(
            (2 * scale, top * scale, 365 * scale, (top + 126) * scale),
            radius=12 * scale,
            fill=(247, 251, 255, 255),
            outline=(22, 139, 255, 255),
            width=2 * scale,
        )
    image.resize(size, Image.Resampling.LANCZOS).save(RESOURCE_DIR / CARDS_NAME, optimize=True)


def textview(caption: str, control_id: int, text: str, position: dict, font_size: int = 20) -> dict:
    return {
        "alignment": 37,
        "caption": caption,
        "colorTab": {"color0": TEXT},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": font_size,
        "id": control_id,
        "position": position,
        "text": text,
        "touchable": False,
    }


def button(caption: str, control_id: int, text: str, position: dict, primary: bool = False) -> dict:
    return {
        "alignment": 37,
        "beepEnable": True,
        "bgColorTab": {
            "color0": BLUE if primary else PANEL,
            "color1": BLUE_DARK if primary else PRESSED,
        },
        "caption": caption,
        "colorTab": {
            "color0": WHITE if primary else BLUE_DARK,
            "color1": WHITE if primary else BLUE_DARK,
        },
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 22,
        "id": control_id,
        "position": position,
        "text": text,
    }


def style_edit(control: dict, position: dict) -> None:
    control.pop("bgColorTab", None)
    control["alignment"] = 37
    control["backgroundPic"] = "system_time_edit_110x58.png"
    control["colorTab"] = {"color0": BLUE_DARK}
    control["fontSize"] = 24
    control["hintTextColor"] = BLUE_DARK
    control["position"] = position
    control["visible"] = True


def main() -> None:
    build_cards()
    data, header, _ = decode_ftu(UI_PATH)
    root = find_caption(data, "Window1")

    cards = find_caption(root, "DateTimeButton")
    cards["backgroundPic"] = CARDS_NAME
    # Deliberately preserve its current layout.
    if cards["position"] != {"height": 310, "left": 70, "top": 140, "width": 367}:
        raise RuntimeError(f"unexpected DateTimeButton layout: {cards['position']}")

    controls = {}
    for caption in (
        "YearEditText", "MonthEditText", "DayEditText",
        "HourEditText", "MinEditText", "SecEditText",
        "TextView3", "TextView4", "TextView5", "TextView6",
    ):
        controls[caption] = pop_caption(root, caption)

    date_window = {
        "backgroundPic": "system_time_dialog_560x260.png",
        "beepEnable": True,
        "caption": "DatePickerWindow",
        "id": 110010,
        "modal": True,
        "position": {"height": 260, "left": 232, "top": 170, "width": 560},
        "visible": False,
        "textview__title": textview(
            "DatePickerMonth", 50020, "设置日期",
            {"height": 44, "left": 150, "top": 12, "width": 260}, 28,
        ),
    }
    date_fields = (
        ("YearEditText", "年", 45, 110),
        ("MonthEditText", "月", 225, 110),
        ("DayEditText", "日", 405, 110),
    )
    for index, (caption, label, left, width) in enumerate(date_fields):
        edit = controls[caption]
        style_edit(edit, {"height": 58, "left": left, "top": 88, "width": width})
        date_window[f"edittext__{index}"] = edit
        date_window[f"textview__{index}"] = textview(
            f"DateInputLabel{index}", 50050 + index, label,
            {"height": 28, "left": left, "top": 58, "width": width}, 18,
        )
    date_window["button__cancel"] = button(
        "DateCancelButton", 20014, "取消",
        {"height": 48, "left": 110, "top": 190, "width": 140},
    )
    date_window["button__done"] = button(
        "DateDoneButton", 20015, "确定",
        {"height": 48, "left": 310, "top": 190, "width": 140}, True,
    )

    time_window = {
        "backgroundPic": "time_picker_panel_520x260.png",
        "beepEnable": True,
        "caption": "TimePickerWindow",
        "id": 110011,
        "modal": True,
        "position": {"height": 260, "left": 252, "top": 170, "width": 520},
        "visible": False,
        "textview__title": textview(
            "TimePickerTitle", 50040, "设置时间",
            {"height": 44, "left": 130, "top": 12, "width": 260}, 28,
        ),
    }
    time_fields = (
        ("HourEditText", "TextView4", "时", 34),
        ("MinEditText", "TextView5", "分", 200),
        ("SecEditText", "TextView6", "秒", 366),
    )
    for index, (edit_caption, label_caption, label, left) in enumerate(time_fields):
        edit = controls[edit_caption]
        style_edit(edit, {"height": 58, "left": left, "top": 88, "width": 110})
        time_window[f"edittext__{index}"] = edit
        label_control = controls[label_caption]
        label_control.update(textview(
            label_caption, label_control["id"], label,
            {"height": 28, "left": left, "top": 58, "width": 110}, 18,
        ))
        time_window[f"textview__{index}"] = label_control
    time_window["button__cancel"] = button(
        "TimeCancelButton", 20016, "取消",
        {"height": 48, "left": 90, "top": 190, "width": 140},
    )
    time_window["button__done"] = button(
        "TimeDoneButton", 20017, "确定",
        {"height": 48, "left": 290, "top": 190, "width": 140}, True,
    )

    controls["TextView3"]["visible"] = False
    root["textview__unused_date_suffix"] = controls["TextView3"]
    root["window__date_picker"] = date_window
    root["window__time_picker"] = time_window

    # Reinsert the row buttons last so their transparent hit areas sit above the card background.
    date_row = pop_caption(root, "DateRowButton")
    time_row = pop_caption(root, "TimeRowButton")
    date_row["bgColorTab"] = {"color0": -1, "color1": PRESSED}
    time_row["bgColorTab"] = {"color0": -1, "color1": PRESSED}
    root["button__date_row_hit"] = date_row
    root["button__time_row_hit"] = time_row

    UI_PATH.write_bytes(encode_ftu(data, header))

    verified, _, _ = decode_ftu(UI_PATH)
    for caption in (
        "DatePickerWindow", "TimePickerWindow", "YearEditText",
        "MonthEditText", "DayEditText", "HourEditText", "MinEditText",
        "SecEditText", "DateRowButton", "TimeRowButton",
    ):
        find_caption(verified, caption)
    print(f"updated {UI_PATH}")


if __name__ == "__main__":
    main()
