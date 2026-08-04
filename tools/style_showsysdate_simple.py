#!/usr/bin/env python3
"""Build the editable system date/time page used by the flashed project."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "showsysdate.ftu"
RESOURCE_DIR = ROOT / "resources"
OUTER_NAME = "time_settings_outer_600x310.png"
DATE_CARD_NAME = "time_date_card_596x128.png"
TIME_CARD_NAME = "time_time_card_596x128.png"
SYNC_CARD_NAME = "time_sync_card_260x96.png"
HOUR24_CARD_NAME = "time_24hour_card_260x96.png"
DIALOG_NAME = "system_time_dialog_560x260.png"
EDIT_NAME = "system_time_edit_110x58.png"
BLUE = 0x168BFF
BLUE_DARK = 0x005BBB
TEXT = 0x111318


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


def build_cards() -> None:
    scale = 4

    def rounded_asset(name: str, size: tuple[int, int], radius: int = 14) -> None:
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
        image.resize(size, Image.Resampling.LANCZOS).save(
            RESOURCE_DIR / name, optimize=True
        )

    rounded_asset(OUTER_NAME, (600, 310), 16)
    rounded_asset(DATE_CARD_NAME, (596, 128))
    rounded_asset(TIME_CARD_NAME, (596, 128))
    rounded_asset(SYNC_CARD_NAME, (260, 96))
    rounded_asset(HOUR24_CARD_NAME, (260, 96))
    rounded_asset(DIALOG_NAME, (560, 260), 18)
    rounded_asset(EDIT_NAME, (110, 58), 10)


def transparent_row(caption: str, control_id: int, top: int, background: str) -> dict:
    return {
        "alignment": 37,
        "beepEnable": True,
        "bgColorTab": {"color0": -1, "color1": 0xEAF6FF},
        "backgroundPic": background,
        "caption": caption,
        "colorTab": {"color0": 0, "color1": 0},
        "fontSize": 20,
        "id": control_id,
        "position": {"height": 128, "left": 72, "top": top, "width": 596},
        "text": "",
    }


def switch_button(caption: str, control_id: int, left: int, top: int) -> dict:
    return {
        "alignment": 37,
        "beepEnable": True,
        "caption": caption,
        "colorTab": {"color0": BLUE_DARK, "color1": BLUE_DARK},
        "iconPosition": {"height": 65, "left": 0, "top": 0, "width": 65},
        "id": control_id,
        "picTab": {
            "pic0": "window3_cycle_toggle_off_65.png",
            "pic2": "window3_cycle_toggle_on_65.png",
            "pic4": "window3_cycle_toggle_on_invalid_65.png",
        },
        "position": {"height": 65, "left": left, "top": top, "width": 65},
        "text": "",
    }


def text_view(caption: str, control_id: int, text: str, position: dict, **extra: object) -> dict:
    control = {
        "alignment": 37,
        "caption": caption,
        "colorTab": {"color0": TEXT},
        "fontSize": 20,
        "id": control_id,
        "position": position,
        "text": text,
        "touchable": False,
    }
    control.update(extra)
    return control


def dialog_button(caption: str, control_id: int, text: str, left: int) -> dict:
    return {
        "alignment": 37,
        "beepEnable": True,
        "bgColorTab": {"color0": -1, "color1": 0xEAF6FF},
        "caption": caption,
        "colorTab": {"color0": BLUE_DARK, "color1": BLUE_DARK},
        "fontSize": 20,
        "id": control_id,
        "position": {"height": 48, "left": left, "top": 194, "width": 110},
        "text": text,
    }


def style_edit(edit: dict, left: int, top: int) -> None:
    edit.update({
        "alignment": 37,
        "backgroundPic": EDIT_NAME,
        "colorTab": {"color0": BLUE_DARK},
        "fontSize": 22,
        "hintTextColor": BLUE_DARK,
        "position": {"height": 58, "left": left, "top": top, "width": 110},
        "visible": True,
    })


def main() -> None:
    build_cards()
    data, header, _ = decode_ftu(UI_PATH)
    root = find_caption(data, "Window1")
    controls = {
        caption: find_caption(data, caption)
        for caption in (
            "sys_back", "TextView7", "TextView1", "TextView2", "TextView3",
            "TextView4", "TextView5", "TextView6", "TextDate", "TextTime",
            "TextWeek", "YearEditText", "MonthEditText", "DayEditText",
            "HourEditText", "MinEditText", "SecEditText",
        )
    }

    root.clear()
    root.update({
        "backgroundColor": 0xFFFFFF,
        "backgroundPic": "wifi_page_rounded_1024x600.png",
        "beepEnable": True,
        "caption": "Window1",
        "id": 110001,
        "position": {"height": 600, "left": 0, "top": 0, "width": 1024},
    })

    controls["TextView1"].update({
        "alignment": 36,
        "colorTab": {"color0": TEXT},
        "fontSize": 30,
        "text": "\u8bbe\u7f6e\u65e5\u671f",
        "position": {"height": 44, "left": 110, "top": 150, "width": 260},
    })
    controls["TextDate"].update({
        "alignment": 36,
        "colorTab": {"color0": BLUE_DARK},
        "fontSize": 25,
        "touchable": False,
        "position": {"height": 44, "left": 110, "top": 202, "width": 500},
    })
    controls["TextView2"].update({
        "alignment": 36,
        "colorTab": {"color0": TEXT},
        "fontSize": 30,
        "text": "\u8bbe\u7f6e\u65f6\u95f4",
        "position": {"height": 44, "left": 110, "top": 310, "width": 260},
    })
    controls["TextTime"].update({
        "alignment": 36,
        "colorTab": {"color0": BLUE_DARK},
        "fontSize": 25,
        "touchable": False,
        "position": {"height": 44, "left": 110, "top": 362, "width": 500},
    })
    controls["TextWeek"]["visible"] = False
    for caption in ("TextView3", "TextView4", "TextView5", "TextView6"):
        controls[caption]["visible"] = False

    root["button__17"] = controls["sys_back"]
    root["textview__18"] = controls["TextView7"]
    # One visible left frame containing two independently editable rows.
    root["window__19"] = {
        "backgroundPic": OUTER_NAME,
        "beepEnable": False,
        "caption": "DateTimeButton",
        "id": 110012,
        "position": {"height": 310, "left": 70, "top": 140, "width": 600},
    }
    root["button__20"] = transparent_row(
        "DateRowButton", 20010, 142, DATE_CARD_NAME
    )
    root["button__21"] = transparent_row(
        "TimeRowButton", 20011, 302, TIME_CARD_NAME
    )

    root["button__22"] = {
        "alignment": 37,
        "backgroundPic": SYNC_CARD_NAME,
        "beepEnable": True,
        "bgColorTab": {"color0": -1, "color1": 0xEAF6FF},
        "caption": "SyncTimeButton",
        "colorTab": {"color0": BLUE_DARK, "color1": BLUE_DARK},
        "fontSize": 20,
        "iconPosition": {"height": 65, "left": 185, "top": 15, "width": 65},
        "id": 20018,
        "picTab": {
            "pic0": "window3_cycle_toggle_off_65.png",
            "pic2": "window3_cycle_toggle_on_65.png",
            "pic4": "window3_cycle_toggle_on_invalid_65.png",
        },
        "position": {"height": 96, "left": 700, "top": 175, "width": 260},
        "text": "\u540c\u6b65\u65f6\u95f4",
        "textPosition": {"height": 96, "left": 16, "top": 0, "width": 160},
    }
    root["button__23"] = {
        "alignment": 37,
        "backgroundPic": HOUR24_CARD_NAME,
        "beepEnable": True,
        "bgColorTab": {"color0": -1, "color1": 0xEAF6FF},
        "caption": "TwentyFourHourButton",
        "colorTab": {"color0": BLUE_DARK, "color1": BLUE_DARK},
        "fontSize": 20,
        "id": 20019,
        "position": {"height": 96, "left": 700, "top": 315, "width": 260},
        "text": "\u4f7f\u752824\u5c0f\u65f6\u5236",
        "textPosition": {"height": 96, "left": 16, "top": 0, "width": 160},
    }
    root["button__24"] = switch_button(
        "TwentyFourHourSwitchButton", 20020, 885, 330
    )

    # Compact editable dialogs.  The existing activity logic discovers these
    # controls by ID and handles confirm/cancel without changing page IDs.
    date_window = {
        "backgroundPic": DIALOG_NAME,
        "beepEnable": True,
        "caption": "DatePickerWindow",
        "id": 110010,
        "modal": True,
        "position": {"height": 260, "left": 232, "top": 170, "width": 560},
        "visible": False,
    }
    date_window["textview__title"] = text_view(
        "DatePickerTitle", 50021, "\u8bbe\u7f6e\u65e5\u671f",
        {"height": 38, "left": 24, "top": 14, "width": 180},
        fontSize=24,
    )
    date_window["textview__month"] = text_view(
        "DatePickerMonth", 50020, "2026\u5e747\u6708",
        {"height": 30, "left": 350, "top": 20, "width": 180},
        colorTab={"color0": BLUE_DARK}, fontSize=18,
    )
    for caption, left in (
        ("YearEditText", 34), ("MonthEditText", 184), ("DayEditText", 334)
    ):
        style_edit(controls[caption], left, 74)
    date_window["edittext__year"] = controls["YearEditText"]
    date_window["edittext__month"] = controls["MonthEditText"]
    date_window["edittext__day"] = controls["DayEditText"]
    date_window["textview__year_unit"] = text_view(
        "DateYearUnit", 50022, "\u5e74", {"height": 58, "left": 145, "top": 74, "width": 36},
        colorTab={"color0": BLUE_DARK}, fontSize=22,
    )
    date_window["textview__month_unit"] = text_view(
        "DateMonthUnit", 50023, "\u6708", {"height": 58, "left": 295, "top": 74, "width": 36},
        colorTab={"color0": BLUE_DARK}, fontSize=22,
    )
    date_window["textview__day_unit"] = text_view(
        "DateDayUnit", 50024, "\u65e5", {"height": 58, "left": 445, "top": 74, "width": 36},
        colorTab={"color0": BLUE_DARK}, fontSize=22,
    )
    date_window["button__cancel"] = dialog_button(
        "DateCancelButton", 20014, "\u53d6\u6d88", 150
    )
    date_window["button__done"] = dialog_button(
        "DateDoneButton", 20015, "\u786e\u5b9a", 300
    )

    time_window = {
        "backgroundPic": DIALOG_NAME,
        "beepEnable": True,
        "caption": "TimePickerWindow",
        "id": 110011,
        "modal": True,
        "position": {"height": 260, "left": 232, "top": 170, "width": 560},
        "visible": False,
    }
    time_window["textview__title"] = text_view(
        "TimePickerTitle", 50040, "\u8bbe\u7f6e\u65f6\u95f4",
        {"height": 38, "left": 24, "top": 14, "width": 180},
        fontSize=24,
    )
    for caption, left in (
        ("HourEditText", 34), ("MinEditText", 184), ("SecEditText", 334)
    ):
        style_edit(controls[caption], left, 74)
    time_window["edittext__hour"] = controls["HourEditText"]
    time_window["edittext__min"] = controls["MinEditText"]
    time_window["edittext__sec"] = controls["SecEditText"]
    for caption, control_id, text, left in (
        ("TimeHourUnit", 50041, "\u65f6", 145),
        ("TimeMinUnit", 50042, "\u5206", 295),
        ("TimeSecUnit", 50043, "\u79d2", 445),
    ):
        time_window[f"textview__{caption}"] = text_view(
            caption, control_id, text,
            {"height": 58, "left": left, "top": 74, "width": 36},
            colorTab={"color0": BLUE_DARK}, fontSize=22,
        )
    time_window["button__cancel"] = dialog_button(
        "TimeCancelButton", 20016, "\u53d6\u6d88", 150
    )
    time_window["button__done"] = dialog_button(
        "TimeDoneButton", 20017, "\u786e\u5b9a", 300
    )
    root["window__date_picker"] = date_window
    root["window__time_picker"] = time_window

    root["textview__25"] = controls["TextView1"]
    root["textview__26"] = controls["TextDate"]
    root["textview__27"] = controls["TextView2"]
    root["textview__28"] = controls["TextTime"]
    root["textview__29"] = controls["TextWeek"]
    for key, caption in (
        ("textview__30", "TextView3"), ("textview__31", "TextView4"),
        ("textview__32", "TextView5"), ("textview__33", "TextView6"),
    ):
        root[key] = controls[caption]

    for key in list(data):
        if key.startswith("window__") and key != "window__16":
            del data[key]
    data["window__16"] = root
    UI_PATH.write_bytes(encode_ftu(data, header))
    print(f"styled {UI_PATH}")


if __name__ == "__main__":
    main()
