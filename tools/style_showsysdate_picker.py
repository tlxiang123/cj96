#!/usr/bin/env python3
"""Build the system date/time page and its local picker dialogs."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "showsysdate.ftu"
RESOURCE_DIR = ROOT / "resources"

BLUE = 0x168BFF
BLUE_DARK = 0x005BBB
TEXT = 0x111318
MUTED = 0x526579
PANEL = 0xF7FBFF
PRESSED = 0xEAF6FF
WHITE = 0xFFFFFF

MAIN_PANEL_NAME = "time_settings_rows_760x270.png"
DATE_PANEL_NAME = "date_picker_panel_720x460.png"
TIME_PANEL_NAME = "time_picker_panel_520x260.png"
EDIT_BACKGROUND_NAME = "w2_name_edit_204x61.png"


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


def control_signature(node: object) -> list[tuple[str, int | None]]:
    result: list[tuple[str, int | None]] = []
    if isinstance(node, dict):
        if "caption" in node:
            result.append((str(node["caption"]), node.get("id")))
        for value in node.values():
            result.extend(control_signature(value))
    elif isinstance(node, list):
        for value in node:
            result.extend(control_signature(value))
    return result


def build_panel(
    path: Path,
    size: tuple[int, int],
    dividers: tuple[int, ...] = (),
    radius: int = 16,
) -> None:
    scale = 4
    width, height = size
    image = Image.new("RGBA", (width * scale, height * scale), (255, 255, 255, 0))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (2 * scale, 2 * scale, width * scale - 3, height * scale - 3),
        radius=radius * scale,
        fill=(247, 251, 255, 255),
        outline=(22, 139, 255, 255),
        width=2 * scale,
    )
    for divider in dividers:
        draw.line(
            (22 * scale, divider * scale, (width - 22) * scale, divider * scale),
            fill=(202, 224, 242, 255),
            width=scale,
        )
    image.resize(size, Image.Resampling.LANCZOS).save(path, optimize=True)


def textview(caption: str, control_id: int, text: str, position: dict, **extra: object) -> dict:
    control = {
        "alignment": 37,
        "caption": caption,
        "colorTab": {"color0": TEXT},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 20,
        "id": control_id,
        "position": position,
        "text": text,
        "touchable": False,
    }
    control.update(extra)
    return control


def button(caption: str, control_id: int, text: str, position: dict, **extra: object) -> dict:
    control = {
        "alignment": 37,
        "beepEnable": True,
        "bgColorTab": {"color0": PANEL, "color1": PRESSED},
        "caption": caption,
        "colorTab": {"color0": TEXT, "color1": TEXT},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 20,
        "id": control_id,
        "position": position,
        "text": text,
    }
    control.update(extra)
    return control


def style_edit(edit: dict, position: dict, font_size: int = 24) -> None:
    edit.pop("bgColorTab", None)
    edit["alignment"] = 37
    edit["backgroundPic"] = EDIT_BACKGROUND_NAME
    edit["colorTab"] = {"color0": BLUE_DARK}
    edit["hintTextColor"] = BLUE_DARK
    edit["fontSize"] = font_size
    edit["position"] = position


def main() -> None:
    build_panel(RESOURCE_DIR / MAIN_PANEL_NAME, (760, 270), (135,))
    build_panel(RESOURCE_DIR / DATE_PANEL_NAME, (720, 460))
    build_panel(RESOURCE_DIR / TIME_PANEL_NAME, (520, 260))

    data, header, _ = decode_ftu(UI_PATH)
    original_signature = control_signature(data)
    root = find_caption(data, "Window1")

    existing = {
        caption: find_caption(data, caption)
        for caption in (
            "sys_back", "TextView7", "TextTime", "TextDate", "TextWeek",
            "YearEditText", "MonthEditText", "DayEditText", "HourEditText",
            "MinEditText", "SecEditText", "TextView1", "TextView2",
            "TextView3", "TextView4", "TextView5", "TextView6",
        )
    }

    root["backgroundColor"] = WHITE
    root["backgroundPic"] = "wifi_page_rounded_1024x600.png"
    root["position"] = {"height": 600, "left": 0, "top": 0, "width": 1024}

    # Main two-row settings surface.
    rows_panel = {
        "backgroundPic": MAIN_PANEL_NAME,
        "beepEnable": False,
        "caption": "TimeSettingsRowsPanel",
        "id": 110012,
        "position": {"height": 270, "left": 132, "top": 158, "width": 760},
    }
    date_row = button(
        "DateRowButton", 20010, "",
        {"height": 133, "left": 134, "top": 160, "width": 756},
        bgColorTab={"color0": -1, "color1": PRESSED},
    )
    time_row = button(
        "TimeRowButton", 20011, "",
        {"height": 133, "left": 134, "top": 295, "width": 756},
        bgColorTab={"color0": -1, "color1": PRESSED},
    )
    existing["TextView1"].update({
        "alignment": 36, "fontSize": 30, "text": "设置日期",
        "colorTab": {"color0": TEXT},
        "position": {"height": 46, "left": 178, "top": 174, "width": 260},
    })
    existing["TextDate"].update({
        "alignment": 36, "fontSize": 25, "colorTab": {"color0": BLUE_DARK},
        "position": {"height": 46, "left": 178, "top": 225, "width": 560},
    })
    existing["TextView2"].update({
        "alignment": 36, "fontSize": 30, "text": "设置时间",
        "colorTab": {"color0": TEXT},
        "position": {"height": 46, "left": 178, "top": 309, "width": 260},
    })
    existing["TextTime"].update({
        "alignment": 36, "fontSize": 25, "colorTab": {"color0": BLUE_DARK},
        "position": {"height": 46, "left": 178, "top": 360, "width": 560},
    })
    existing["TextWeek"]["visible"] = False

    date_window = {
        "backgroundPic": DATE_PANEL_NAME,
        "beepEnable": True,
        "caption": "DatePickerWindow",
        "id": 110010,
        "modal": True,
        "position": {"height": 460, "left": 152, "top": 115, "width": 720},
        "visible": False,
    }
    date_window["button__1"] = button(
        "DatePrevButton", 20012, "<",
        {"height": 54, "left": 24, "top": 14, "width": 64}, fontSize=36,
    )
    date_window["textview__2"] = textview(
        "DatePickerMonth", 50020, "2026年7月",
        {"height": 54, "left": 180, "top": 14, "width": 360}, fontSize=30,
    )
    date_window["button__3"] = button(
        "DateNextButton", 20013, ">",
        {"height": 54, "left": 632, "top": 14, "width": 64}, fontSize=36,
    )

    weekdays = ("日", "一", "二", "三", "四", "五", "六")
    for index, weekday in enumerate(weekdays):
        date_window[f"textview__weekday_{index}"] = textview(
            f"DateWeekday{index}", 50030 + index, weekday,
            {"height": 32, "left": 45 + index * 90, "top": 76, "width": 90},
            colorTab={"color0": MUTED}, fontSize=18,
        )

    for index in range(42):
        row, column = divmod(index, 7)
        date_window[f"button__day_{index}"] = button(
            f"DateDay{index}", 20100 + index, "",
            {
                "height": 44,
                "left": 45 + column * 90,
                "top": 112 + row * 44,
                "width": 90,
            },
            bgColorTab={"color0": PANEL, "color1": PRESSED, "color2": BLUE},
            colorTab={"color0": TEXT, "color1": TEXT, "color2": WHITE},
            fontSize=20,
        )

    date_window["button__cancel"] = button(
        "DateCancelButton", 20014, "取消",
        {"height": 48, "left": 170, "top": 402, "width": 140},
        colorTab={"color0": BLUE_DARK, "color1": BLUE_DARK}, fontSize=22,
    )
    date_window["button__done"] = button(
        "DateDoneButton", 20015, "完成",
        {"height": 48, "left": 410, "top": 402, "width": 140},
        bgColorTab={"color0": BLUE, "color1": BLUE_DARK},
        colorTab={"color0": WHITE, "color1": WHITE}, fontSize=22,
    )

    time_window = {
        "backgroundPic": TIME_PANEL_NAME,
        "beepEnable": True,
        "caption": "TimePickerWindow",
        "id": 110011,
        "modal": True,
        "position": {"height": 260, "left": 252, "top": 170, "width": 520},
        "visible": False,
    }
    time_window["textview__title"] = textview(
        "TimePickerTitle", 50040, "设置时间",
        {"height": 48, "left": 130, "top": 12, "width": 260}, fontSize=28,
    )
    time_fields = (
        ("HourEditText", "TextView4", "时", 34),
        ("MinEditText", "TextView5", "分", 200),
        ("SecEditText", "TextView6", "秒", 366),
    )
    for edit_caption, label_caption, label_text, left in time_fields:
        edit = existing[edit_caption]
        style_edit(edit, {"height": 61, "left": left, "top": 92, "width": 120}, 24)
        label = existing[label_caption]
        label.update({
            "alignment": 37, "fontSize": 18, "text": label_text,
            "colorTab": {"color0": MUTED},
            "position": {"height": 28, "left": left, "top": 62, "width": 120},
        })
        time_window[f"edittext__{edit_caption}"] = edit
        time_window[f"textview__{label_caption}"] = label

    time_window["button__cancel"] = button(
        "TimeCancelButton", 20016, "取消",
        {"height": 48, "left": 90, "top": 194, "width": 140},
        colorTab={"color0": BLUE_DARK, "color1": BLUE_DARK}, fontSize=22,
    )
    time_window["button__done"] = button(
        "TimeDoneButton", 20017, "完成",
        {"height": 48, "left": 290, "top": 194, "width": 140},
        bgColorTab={"color0": BLUE, "color1": BLUE_DARK},
        colorTab={"color0": WHITE, "color1": WHITE}, fontSize=22,
    )

    for caption in ("YearEditText", "MonthEditText", "DayEditText", "TextView3"):
        existing[caption]["visible"] = False

    # Remove the old top-level controls. They must be children of Window1 so
    # the full-page background cannot cover them.
    for key in list(data):
        value = data[key]
        if isinstance(value, dict) and value is not root and value.get("caption"):
            del data[key]

    root["window__rows_panel"] = rows_panel
    root["button__date_row"] = date_row
    root["button__time_row"] = time_row
    root["textview__date_label"] = existing["TextView1"]
    root["textview__date_value"] = existing["TextDate"]
    root["textview__time_label"] = existing["TextView2"]
    root["textview__time_value"] = existing["TextTime"]
    root["textview__week"] = existing["TextWeek"]
    root["edittext__year"] = existing["YearEditText"]
    root["edittext__month"] = existing["MonthEditText"]
    root["edittext__day"] = existing["DayEditText"]
    root["textview__unused"] = existing["TextView3"]
    root["window__date_picker"] = date_window
    root["window__time_picker"] = time_window

    UI_PATH.write_bytes(encode_ftu(data, header))
    verified, _, _ = decode_ftu(UI_PATH)
    verified_signature = control_signature(verified)
    for item in original_signature:
        if item not in verified_signature:
            raise RuntimeError(f"lost existing control: {item}")
    for caption in (
        "DateRowButton", "TimeRowButton", "DatePickerWindow",
        "TimePickerWindow", "DatePickerMonth", "DateDay41",
    ):
        find_caption(verified, caption)
    print(f"styled {UI_PATH}")


if __name__ == "__main__":
    main()
