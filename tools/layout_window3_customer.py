#!/usr/bin/env python3
"""Lay out Window3 for clear program, schedule, mode, and runtime workflows."""

from __future__ import annotations

from pathlib import Path

from PIL import Image

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "main.ftu"
TOUCHABLE_UI = True

SCROLL_CAPTION = "Window3ScrollWindow"
CONTENT_CAPTION = "Window3ScrollContentWindow"
CONTENT_HEIGHT = 885

REGION_LAYOUT = {
    "Window3Region1Window": (10, 10, 220, 100),
    "Window3Region2Window": (240, 10, 470, 100),
    "Window3Region3Window": (720, 10, 277, 100),
    "Window3Region4Window": (10, 120, 987, 145),
    "Window3Region5Window": (10, 275, 488, 290),
    "Window3Region6Window": (509, 275, 488, 290),
    "Window3Region7Window": (10, 575, 987, 300),
}

WEEKDAY_BUTTONS = (
    "EverDayButton", "SundayButton", "MondayButton", "TuesdayButton",
    "WednesdayButton", "ThursdayButton", "FridayButton", "SaturdayButton",
)

VALUE_PICKER_BUTTONS = (
    ("W3StartTime1PickerButton", 21001, "StartTimeHour1EditText"),
    ("W3StartTime2PickerButton", 21002, "StartTimeHour2EditText"),
    ("W3StartTime3PickerButton", 21003, "StartTimeHour3EditText"),
    ("W3StartTime4PickerButton", 21004, "StartTimeHour4EditText"),
    ("W3IntervalDayPickerButton", 21005, "IntervalDayEditText"),
    ("W3AfterDayPickerButton", 21006, "AfterDayEditText"),
)


def resource_average_blue(name: str) -> int:
    image = Image.open(ROOT / "resources" / name).convert("RGBA")
    pixel_source = (
        image.get_flattened_data()
        if hasattr(image, "get_flattened_data")
        else image.getdata()
    )
    pixels = [
        pixel for pixel in pixel_source
        if pixel[3] > 160 and pixel[0] < 90 and pixel[2] > 120
    ]
    if not pixels:
        return 0x168BFF
    red = round(sum(pixel[0] for pixel in pixels) / len(pixels))
    green = round(sum(pixel[1] for pixel in pixels) / len(pixels))
    blue = round(sum(pixel[2] for pixel in pixels) / len(pixels))
    return (red << 16) | (green << 8) | blue


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


def position(control: dict, left: int, top: int, width: int, height: int) -> None:
    control["position"] = {"height": height, "left": left, "top": top, "width": width}


def picture_control(caption: str, control_id: int, picture: str, box: tuple[int, int, int, int]) -> dict:
    left, top, width, height = box
    return {
        "alignment": 37,
        "caption": caption,
        "colorTab": {"color0": 0},
        "id": control_id,
        "picTab": {"pic0": picture},
        "touchable": False,
        "position": {"height": height, "left": left, "top": top, "width": width},
    }


def upsert_picture(region: dict, key: str, caption: str, control_id: int, picture: str, box: tuple[int, int, int, int]) -> None:
    for existing_key, value in list(region.items()):
        if isinstance(value, dict) and value.get("caption") == caption:
            del region[existing_key]
    region[key] = picture_control(caption, control_id, picture, box)


def picker_button(caption: str, control_id: int, box: tuple[int, int, int, int], picture: str) -> dict:
    left, top, width, height = box
    return {
        "alignment": 37,
        "caption": caption,
        "id": control_id,
        "beepEnable": True,
        "colorTab": {"color0": 1471929},
        "fontSize": 20,
        "text": "--:--" if "time" in caption.lower() else "--",
        "position": {"height": height, "left": left, "top": top, "width": width},
    }


def upsert_picker_button(region: dict, caption: str, control_id: int, source_caption: str, box: tuple[int, int, int, int], picture: str = "window3_number_empty.png") -> None:
    source = find_control(region, source_caption)
    source["visible"] = False
    for key, value in list(region.items()):
        if isinstance(value, dict) and value.get("caption") == caption:
            del region[key]
    region[f"button__{control_id}"] = picker_button(caption, control_id, box, picture)


def layout_start_times(region: dict) -> None:
    title = find_control(region, "Button14")
    position(title, 8, 18, 108, 108)
    title.pop("text", None)
    title["picTab"] = {"pic0": "window3_start_time_108.png"}

    groups = (
        ("Button10", "StartTimeHour1EditText", "TextView8", "StartTimeMin1EditText", 125),
        ("Button11", "StartTimeHour2EditText", "TextView9", "StartTimeMin2EditText", 335),
        ("Button12", "StartTimeHour3EditText", "TextView10", "StartTimeMin3EditText", 545),
        ("Button13", "StartTimeHour4EditText", "TextView11", "StartTimeMin4EditText", 755),
    )
    for alarm, hour, colon, minute, left in groups:
        position(find_control(region, alarm), left, 25, 60, 60)
        position(find_control(region, hour), left + 65, 37, 50, 40)
        position(find_control(region, colon), left + 115, 29, 26, 45)
        position(find_control(region, minute), left + 141, 37, 50, 40)
        if TOUCHABLE_UI:
            continue
        find_control(region, hour)["visible"] = False
        find_control(region, minute)["visible"] = False
        find_control(region, colon)["visible"] = False
    if not TOUCHABLE_UI:
        for index, (_, _, _, minute, left) in enumerate(groups):
            caption, control_id, source_caption = VALUE_PICKER_BUTTONS[index]
            find_control(region, minute)["visible"] = False
            upsert_picker_button(
                region, caption, control_id, source_caption,
                (left + 65, 30, 126, 55), "",
            )


def layout_week_mode(region: dict) -> None:
    upsert_picture(
        region, "button__200", "Window3WeekModeIcon", 20062,
        "window3_week_mode_108.png", (8, 15, 108, 108),
    )
    position(find_control(region, "WeekModeButton"), 135, 20, 190, 60)
    weekday_blue = resource_average_blue("window3_week_mode_108.png")
    for index, caption in enumerate(WEEKDAY_BUTTONS):
        column = index % 4
        row = index // 4
        button = find_control(region, caption)
        position(button, 20 + column * 115, 145 + row * 60, 108, 45)
        if "colorTab" in button:
            button["colorTab"] = {
                key: weekday_blue for key in button["colorTab"]
            }


def layout_interval_mode(region: dict) -> None:
    upsert_picture(
        region, "button__201", "Window3IntervalModeIcon", 20063,
        "window3_interval_mode_100.png", (12, 15, 100, 100),
    )
    upsert_picture(
        region, "button__202", "Window3IntervalIcon", 20064,
        "window3_interval_96.png", (30, 125, 96, 96),
    )
    upsert_picture(
        region, "button__203", "Window3DelayIcon", 20065,
        "window3_delay_96.png", (265, 125, 96, 96),
    )
    position(find_control(region, "IntervalModeButton"), 125, 20, 180, 60)
    interval_label = find_control(region, "Button26")
    interval_label["visible"] = False
    interval_label.pop("text", None)
    delay_label = find_control(region, "Button27")
    delay_label["visible"] = False
    delay_label.pop("text", None)
    position(find_control(region, "IntervalDayEditText"), 152, 154, 80, 39)
    position(find_control(region, "AfterDayEditText"), 387, 154, 80, 39)
    for caption, control_id, source_caption in VALUE_PICKER_BUTTONS[4:]:
        if TOUCHABLE_UI:
            continue
        source = find_control(region, source_caption)
        pos = source["position"]
        upsert_picker_button(
            region, caption, control_id, source_caption,
            (pos["left"], pos["top"], pos["width"], pos["height"]),
        )


def layout_runtime_list(region: dict) -> None:
    listview = find_control(region, "RunTimeListView")
    position(listview, 18, 20, 951, 260)
    item = listview["item"]
    position(item, 0, 0, 946, 60)
    subitems = item.get("subItem", [])
    if len(subitems) >= 2:
        position(subitems[0], 0, 0, 240, 60)
        position(subitems[1], 240, 0, 706, 60)
        if "textPosition" in subitems[1]:
            subitems[1]["textPosition"] = {
                "height": 60, "left": 8, "top": 0, "width": 698
            }


def layout_value_picker(window: dict) -> None:
    for key, value in list(window.items()):
        if isinstance(value, dict) and value.get("caption") in (
            "Window3ValuePickerWindow", "W3TimePickerWindow", "W3DayPickerWindow",
        ):
            del window[key]

    if TOUCHABLE_UI:
        return
    window["window__320"] = {
        "backgroundPic": "window3_value_picker_panel_1007x238.png",
        "beepEnable": True,
        "caption": "W3TimePickerWindow",
        "id": 110030,
        "visible": False,
        "position": {"height": 238, "left": 0, "top": 162, "width": 1007},
        "button__321": {
            "alignment": 37,
            "caption": "W3TimePickerCancelButton",
            "colorTab": {"color0": 4805731},
            "fontSize": 24,
            "id": 21011,
            "text": "取消",
            "position": {"height": 70, "left": 160, "top": 163, "width": 160},
        },
        "button__322": {
            "alignment": 37,
            "caption": "W3TimePickerConfirmButton",
            "colorTab": {"color0": 1471929},
            "fontSize": 24,
            "id": 21012,
            "text": "确定",
            "position": {"height": 70, "left": 687, "top": 163, "width": 160},
        },
        "textview__323": {
            "alignment": 37,
            "caption": "W3TimePickerHourUnit",
            "colorTab": {"color0": 4805731},
            "fontSize": 18,
            "id": 50070,
            "text": "小时",
            "touchable": False,
            "position": {"height": 48, "left": 454, "top": 54, "width": 50},
        },
        "textview__324": {
            "alignment": 37,
            "caption": "W3TimePickerMinuteUnit",
            "colorTab": {"color0": 4805731},
            "fontSize": 18,
            "id": 50071,
            "text": "分钟",
            "touchable": False,
            "position": {"height": 48, "left": 758, "top": 54, "width": 50},
        },
        "listview__325": {
            "beepEnable": True,
            "caption": "W3TimePickerHourListView",
            "cols": 1,
            "id": 80010,
            "rows": 3,
            "position": {"height": 150, "left": 300, "top": 5, "width": 145},
            "colSpacing": 0,
            "rowSpacing": 0,
            "item": {
                "alignment": 37,
                "caption": "item",
                "position": {"height": 50, "left": 0, "top": 0, "width": 145},
                "subItem": [{
                    "alignment": 37,
                    "caption": "W3TimePickerHourItem",
                    "colorTab": {"color0": 4805731},
                    "fontSize": 24,
                    "id": 24050,
                    "text": "00",
                    "touchable": True,
                    "position": {"height": 50, "left": 0, "top": 0, "width": 145},
                }],
            },
        },
        "listview__326": {
            "beepEnable": True,
            "caption": "W3TimePickerMinuteListView",
            "cols": 1,
            "id": 80011,
            "rows": 3,
            "position": {"height": 150, "left": 604, "top": 5, "width": 145},
            "colSpacing": 0,
            "rowSpacing": 0,
            "item": {
                "alignment": 37,
                "caption": "item",
                "position": {"height": 50, "left": 0, "top": 0, "width": 145},
                "subItem": [{
                    "alignment": 37,
                    "caption": "W3TimePickerMinuteItem",
                    "colorTab": {"color0": 4805731},
                    "fontSize": 24,
                    "id": 24051,
                    "text": "00",
                    "touchable": True,
                    "position": {"height": 50, "left": 0, "top": 0, "width": 145},
                }],
            },
        },
    }

    window["window__327"] = {
        "backgroundPic": "window3_value_picker_panel_1007x238.png",
        "beepEnable": True,
        "caption": "W3DayPickerWindow",
        "id": 110031,
        "visible": False,
        "position": {"height": 238, "left": 0, "top": 162, "width": 1007},
        "button__328": {
            "alignment": 37,
            "caption": "W3DayPickerCancelButton",
            "colorTab": {"color0": 4805731},
            "fontSize": 24,
            "id": 21013,
            "text": "取消",
            "position": {"height": 70, "left": 160, "top": 163, "width": 160},
        },
        "button__329": {
            "alignment": 37,
            "caption": "W3DayPickerConfirmButton",
            "colorTab": {"color0": 1471929},
            "fontSize": 24,
            "id": 21014,
            "text": "确定",
            "position": {"height": 70, "left": 687, "top": 163, "width": 160},
        },
        "listview__330": {
            "beepEnable": True,
            "caption": "W3DayPickerListView",
            "cols": 1,
            "id": 80012,
            "rows": 3,
            "position": {"height": 150, "left": 431, "top": 5, "width": 145},
            "colSpacing": 0,
            "rowSpacing": 0,
            "item": {
                "alignment": 37,
                "caption": "item",
                "position": {"height": 50, "left": 0, "top": 0, "width": 145},
                "subItem": [{
                    "alignment": 37,
                    "caption": "W3DayPickerItem",
                    "colorTab": {"color0": 4805731},
                    "fontSize": 24,
                    "id": 24052,
                    "text": "01",
                    "touchable": True,
                    "position": {"height": 50, "left": 0, "top": 0, "width": 145},
                }],
            },
        },
    }

    return

    def option_button(caption: str, control_id: int, value: int, top: int, left: int) -> dict:
        return {
            "alignment": 37,
            "caption": caption,
            "colorTab": {"color0": 4805731},
            "fontSize": 20,
            "id": control_id,
            "text": f"{value:02d}",
            "position": {"height": 42, "left": left, "top": top, "width": 104},
        }

    def static_picker(caption: str, control_id: int, base_id: int, count: int) -> dict:
        touch_width = 160
        visual_inset = 28
        content = {
            "backgroundColor": 16776700,
            "caption": f"{caption}Content",
            "id": control_id + 100,
            "position": {"height": count * 42, "left": 0, "top": 0, "width": touch_width},
        }
        for value in range(count):
            content[f"button__{base_id + value}"] = option_button(
                f"{caption}{value:02d}", base_id + value, value, value * 42,
                visual_inset,
            )
        return {
            "caption": caption,
            "dragMaxDis": 200,
            "edgeEffect": 1,
            "id": control_id,
            "orientation": 1,
            "position": {"height": 126, "left": 0, "top": 0, "width": touch_width},
            "window__1": content,
        }

    time_picker = window["window__320"]
    del time_picker["listview__325"]
    del time_picker["listview__326"]
    hour_picker = static_picker("W3TimePickerHourScroll", 32010, 21100, 24)
    hour_picker["position"].update({"left": 0, "top": 12})
    minute_picker = static_picker("W3TimePickerMinuteScroll", 32011, 21200, 60)
    minute_picker["position"].update({"left": 180, "top": 12})
    time_picker["scrollwindow__325"] = hour_picker
    time_picker["scrollwindow__326"] = minute_picker

    day_picker = window["window__327"]
    del day_picker["listview__330"]
    day_scroll = static_picker("W3DayPickerScroll", 32012, 21301, 99)
    day_scroll["position"].update({"left": 100, "top": 12})
    day_picker["scrollwindow__330"] = day_scroll


def patch_ftu() -> None:
    data, header, _ = decode_ftu(FTU_PATH)
    window = find_control(data, "Window3")
    scroll = find_control(window, SCROLL_CAPTION)
    content = find_control(scroll, CONTENT_CAPTION)

    # Preserve all existing colors. Only geometry, hierarchy-adjacent artwork,
    # and scroll content size are changed below.
    content["position"]["height"] = CONTENT_HEIGHT
    scroll["orientation"] = 1
    for caption, (left, top, width, height) in REGION_LAYOUT.items():
        position(find_control(content, caption), left, top, width, height)

    region1 = find_control(content, "Window3Region1Window")
    spray = find_control(region1, "Button15")
    position(spray, 12, 20, 60, 60)
    spray.pop("text", None)
    spray["picTab"] = {"pic0": "window3_spray_program_60.png"}
    cycle = find_control(region1, "CycleButton")
    position(cycle, 90, 20, 122, 60)
    cycle["picTab"] = {
        "pic0": "window3_cycle_toggle_off_65.png",
        "pic2": "window3_cycle_toggle_on_65.png",
    }
    cycle["iconPosition"] = {"height": 65, "left": 30, "top": 0, "width": 65}

    region2 = find_control(content, "Window3Region2Window")
    upsert_picture(
        region2, "button__199", "Window3ProgramIcon", 20061,
        "window3_program_88.png", (4, 6, 88, 88),
    )
    position(find_control(region2, "PervProgButton"), 104, 22, 60, 55)
    position(find_control(region2, "ShowProgEditText"), 169, 20, 226, 60)
    position(find_control(region2, "NextProgButton"), 400, 22, 60, 55)

    region3 = find_control(content, "Window3Region3Window")
    enable = find_control(region3, "OnOffProgTextButton")
    position(enable, 4, 6, 88, 88)
    enable.pop("text", None)
    enable["picTab"] = {
        "pic0": "window3_enable_88.png",
        "pic2": "window3_enable_88.png",
    }
    position(find_control(region3, "OnOffProgButton"), 110, 20, 150, 60)

    layout_start_times(find_control(content, "Window3Region4Window"))
    layout_week_mode(find_control(content, "Window3Region5Window"))
    layout_interval_mode(find_control(content, "Window3Region6Window"))
    layout_runtime_list(find_control(content, "Window3Region7Window"))
    layout_value_picker(window)

    FTU_PATH.write_bytes(encode_ftu(data, header))


def verify() -> None:
    data, _, _ = decode_ftu(FTU_PATH)
    window = find_control(data, "Window3")
    scroll = find_control(window, SCROLL_CAPTION)
    content = find_control(scroll, CONTENT_CAPTION)
    if scroll.get("orientation") != 1 or content["position"]["height"] != CONTENT_HEIGHT:
        raise RuntimeError("Window3 scroll layout verification failed")

    captions = set()
    for region_caption, (_, _, width, height) in REGION_LAYOUT.items():
        region = find_control(content, region_caption)
        for value in region.values():
            if not isinstance(value, dict) or "position" not in value:
                continue
            caption = value.get("caption")
            if caption:
                if caption in captions:
                    raise RuntimeError(f"duplicate Window3 control: {caption}")
                captions.add(caption)
            pos = value["position"]
            if (
                pos["left"] < 0 or pos["top"] < 0
                or pos["left"] + pos["width"] > width
                or pos["top"] + pos["height"] > height
            ):
                raise RuntimeError(f"control outside {region_caption}: {caption}")

    region7 = find_control(content, "Window3Region7Window")
    if region7["position"]["top"] != 575:
        raise RuntimeError("runtime list is not the final Window3 region")


def main() -> None:
    patch_ftu()
    verify()
    print("applied customer-oriented Window3 layout")


if __name__ == "__main__":
    main()
