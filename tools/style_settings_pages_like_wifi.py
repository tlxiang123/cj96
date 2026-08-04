#!/usr/bin/env python3
"""Apply the Wi-Fi page shell to the other TopSet destination pages."""

from __future__ import annotations

import copy
from pathlib import Path

from ftu_style import decode_ftu, encode_ftu
from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
UI_DIR = ROOT / "ui"
WIFI_UI = UI_DIR / "wifisetting.ftu"
MAIN_UI = UI_DIR / "main.ftu"
BACKGROUND_NAME = "wifi_page_rounded_1024x600.png"
BRIGHTNESS_BACKGROUND_NAME = "brightness_seekbar_bg.png"
BRIGHTNESS_PROGRESS_NAME = "brightness_seekbar_progress.png"
BRIGHTNESS_THUMB_NAME = "brightness_seekbar_thumb.png"
EDIT_BACKGROUND_NAME = "w2_name_edit_204x61.png"
ETHERNET_STATIC_PANEL_NAME = "ethernet_static_panel_540x330.png"
ETHERNET_INFO_PANEL_NAME = "ethernet_info_panel_430x235.png"
ETHERNET_VALUE_COLOR = 0x005BBB

PAGE_CONFIG = {
    "ethernetsetting.ftu": {"root": "topWin", "title": "title"},
    "lte4gsetting.ftu": {"root": "Window1", "title": "Textview1"},
    "setdisplay.ftu": {"root": "Window1", "title": "TextView7"},
    "showsysdate.ftu": {"root": "Window1", "title": "TextView7"},
}

HEADER_ICON_CONFIG = {
    "ethernetsetting.ftu": ("title", "topset_ethernet_113.png"),
    "lte4gsetting.ftu": ("Textview1", "topset_4g_113.png"),
    "setdisplay.ftu": ("TextView7", "topset_display_113.png"),
    "showsysdate.ftu": ("TextView7", "topset_time_113.png"),
}
TOPSET_GENERAL_HEADER_SOURCE_NAME = "topset_general_settings_user.png"
TOPSET_GENERAL_HEADER_NAME = "topset_general_settings_header_158x100.png"
TOPSET_GENERAL_HEADER_SIZE = (158, 100)
HEADER_ICON_SIZE = 82

VISUAL_FIELDS = (
    "alignment",
    "bold",
    "colorTab",
    "family",
    "fontSize",
    "iconPosition",
    "picTab",
    "position",
    "textPosition",
)

ETHERNET_TOP_POSITIONS = {
    "connectType": 118,
    "DYNAMIC_IP_BUTTON": 135,
    "STATIC_IP_BUTTON": 336,
    "SAVE_BUTTON": 157,
    "staticIPSetUpWin": 125,
    "saveTipWin": 220,
    "menuTipWin": 170,
}


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


def copy_visual(target: dict, source: dict) -> None:
    for field in VISUAL_FIELDS:
        if field in source:
            target[field] = copy.deepcopy(source[field])


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


def set_top_level_positions(data: dict, positions: dict[str, int]) -> None:
    for value in data.values():
        if not isinstance(value, dict):
            continue
        caption = value.get("caption", "")
        if caption in positions and isinstance(value.get("position"), dict):
            value["position"]["top"] = positions[caption]


def build_brightness_seekbar_assets() -> None:
    width = 650
    height = 40
    line_top = 16
    line_bottom = 23
    radius = 4

    def track(color: tuple[int, int, int, int]) -> Image.Image:
        image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
        ImageDraw.Draw(image).rounded_rectangle(
            (0, line_top, width - 1, line_bottom), radius=radius, fill=color
        )
        return image

    track((69, 153, 245, 255)).save(
        ROOT / "resources" / BRIGHTNESS_BACKGROUND_NAME, optimize=True
    )
    track((0, 102, 255, 255)).save(
        ROOT / "resources" / BRIGHTNESS_PROGRESS_NAME, optimize=True
    )

    # Draw oversized and downsample with Lanczos so the circular thumb has
    # antialiased transparent edges instead of pixel-step staircasing.
    scale = 4
    thumb_large = Image.new("RGBA", (40 * scale, 40 * scale), (0, 0, 0, 0))
    ImageDraw.Draw(thumb_large).ellipse(
        (0, 0, 40 * scale - 1, 40 * scale - 1), fill=(0, 102, 255, 255)
    )
    thumb = thumb_large.resize((40, 40), Image.Resampling.LANCZOS)
    thumb.save(ROOT / "resources" / BRIGHTNESS_THUMB_NAME, optimize=True)


def build_ethernet_panel_assets() -> None:
    scale = 4

    def panel(size: tuple[int, int], dividers: tuple[int, ...] = ()) -> Image.Image:
        width, height = size
        image = Image.new(
            "RGBA", (width * scale, height * scale), (255, 255, 255, 0)
        )
        draw = ImageDraw.Draw(image)
        draw.rounded_rectangle(
            (2 * scale, 2 * scale, width * scale - 3, height * scale - 3),
            radius=14 * scale,
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
        return image.resize(size, Image.Resampling.LANCZOS)

    panel((540, 330)).save(
        ROOT / "resources" / ETHERNET_STATIC_PANEL_NAME, optimize=True
    )
    panel((430, 235), (78, 156)).save(
        ROOT / "resources" / ETHERNET_INFO_PANEL_NAME, optimize=True
    )


def build_header_icon_assets() -> None:
    for source_name in {
        source_name for _, source_name in HEADER_ICON_CONFIG.values()
    }:
        source = Image.open(ROOT / "resources" / source_name).convert("RGBA")
        output_name = source_name.replace("_113.png", "_header_82.png")
        source.resize((HEADER_ICON_SIZE, HEADER_ICON_SIZE), Image.Resampling.LANCZOS).save(
            ROOT / "resources" / output_name, optimize=True
        )


def build_general_header_icon_asset() -> None:
    source = Image.open(
        ROOT / "resources" / TOPSET_GENERAL_HEADER_SOURCE_NAME
    ).convert("RGB")
    pixels: list[tuple[int, int, int, int]] = []
    for red, green, blue in source.getdata():
        blue_strength = max(
            0.0,
            min(1.0, max((blue - red - 18) / 115.0, (blue - green - 5) / 85.0)),
        )
        if blue_strength <= 0.0:
            pixels.append((0, 0, 0, 0))
        else:
            pixels.append((red, green, blue, round(255 * blue_strength)))
    transparent = Image.new("RGBA", source.size, (0, 0, 0, 0))
    transparent.putdata(pixels)
    transparent.resize(
        TOPSET_GENERAL_HEADER_SIZE, Image.Resampling.LANCZOS
    ).save(ROOT / "resources" / TOPSET_GENERAL_HEADER_NAME, optimize=True)


def style_header_icon(title: dict, icon_name: str) -> None:
    title["backgroundPic"] = icon_name.replace("_113.png", "_header_82.png")
    title["text"] = ""
    title["colorTab"] = {"color0": 0x168BFF}
    title["position"] = {
        "height": HEADER_ICON_SIZE,
        "left": 471,
        "top": 9,
        "width": HEADER_ICON_SIZE,
    }
    title.pop("textPosition", None)


def style_page(path: Path, config: dict, style: dict) -> None:
    data, header, _ = decode_ftu(path)
    before_signature = control_signature(data)

    root = find_caption(data, config["root"])
    root["position"] = {"left": 0, "top": 0, "width": 1024, "height": 600}
    root["backgroundColor"] = 0xFFFFFF
    root["backgroundPic"] = BACKGROUND_NAME

    copy_visual(find_caption(data, "sys_back"), find_caption(style, "sys_back"))
    copy_visual(find_caption(data, config["title"]), find_caption(style, "Textview1"))
    if path.name in HEADER_ICON_CONFIG:
        _, icon_name = HEADER_ICON_CONFIG[path.name]
        style_header_icon(find_caption(data, config["title"]), icon_name)

    # The LTE page has the same header controls as Wi-Fi. Keep its IDs/text,
    # but use the already tested switch and menu geometry/artwork.
    for caption in ("ButtonOnOff", "ButtonMenu"):
        try:
            copy_visual(find_caption(data, caption), find_caption(style, caption))
        except KeyError:
            pass

    # Ethernet's first label/radio row was positioned inside the old 83px
    # header. Set absolute top positions so repeated styling is idempotent.
    if path.name == "ethernetsetting.ftu":
        set_top_level_positions(data, ETHERNET_TOP_POSITIONS)

    path.write_bytes(encode_ftu(data, header))
    verified, _, _ = decode_ftu(path)
    if control_signature(verified) != before_signature:
        raise RuntimeError(f"control IDs/captions changed in {path.name}")


def style_ethernet_content(ethernet: dict) -> None:
    static_panel = find_caption(ethernet, "staticIPSetUpWin")
    static_panel["backgroundPic"] = ETHERNET_STATIC_PANEL_NAME
    static_panel["position"] = {
        "height": 330,
        "left": 270,
        "top": 125,
        "width": 540,
    }

    label_positions = {
        "ipAddr": (22, 16),
        "mask": (22, 116),
        "gateWay": (22, 216),
        "firstDNS": (314, 16),
        "secondDNS": (314, 116),
    }
    input_positions = {
        "ipAddrInput": (22, 43),
        "maskInput": (22, 143),
        "gateWayInput": (22, 243),
        "firstDNSInput": (314, 43),
        "secondDNSInput": (314, 143),
    }
    for caption, (left, top) in label_positions.items():
        label = find_caption(ethernet, caption)
        label["position"] = {
            "height": 24,
            "left": left,
            "top": top,
            "width": 204,
        }
        label["fontSize"] = 18
        label["colorTab"] = {"color0": 0x111318}

    for caption, (left, top) in input_positions.items():
        edit = find_caption(ethernet, caption)
        edit.pop("bgColorTab", None)
        edit["backgroundPic"] = EDIT_BACKGROUND_NAME
        edit["alignment"] = 37
        edit["colorTab"] = {"color0": ETHERNET_VALUE_COLOR}
        edit["hintTextColor"] = ETHERNET_VALUE_COLOR
        edit["fontSize"] = 20
        edit["position"] = {
            "height": 61,
            "left": left,
            "top": top,
            "width": 204,
        }

    info_panel = find_caption(ethernet, "menuTipWin")
    info_panel["visible"] = False
    info_rows = (
        ("ipAddrTip", "ipAddress", 25),
        ("macAddrTip", "macAddress", 125),
        ("TextView1", "NetStatus", 225),
    )
    auto_controls: dict[str, dict] = {}
    for label_caption, value_caption, top in info_rows:
        label = find_caption(ethernet, label_caption)
        label["alignment"] = 36
        label["fontSize"] = 19
        label["colorTab"] = {"color0": 0x111318}
        label["position"] = {
            "height": 61,
            "left": 30,
            "top": top,
            "width": 190,
        }

        value = find_caption(ethernet, value_caption)
        value["alignment"] = 37
        value["fontSize"] = 20
        value["colorTab"] = {"color0": ETHERNET_VALUE_COLOR}
        value["backgroundPic"] = EDIT_BACKGROUND_NAME
        value["position"] = {
            "height": 61,
            "left": 150,
            "top": top,
            "width": 204,
        }
        auto_controls[label_caption] = label
        auto_controls[value_caption] = value

    for key in list(info_panel):
        child = info_panel[key]
        if isinstance(child, dict) and child.get("caption") in auto_controls:
            del info_panel[key]
    for key in list(static_panel):
        child = static_panel[key]
        if isinstance(child, dict) and child.get("caption") in auto_controls:
            del static_panel[key]
    for index, control in enumerate(auto_controls.values()):
        static_panel[f"textview__{101 + index}"] = control


def style_setting_button_assets(style: dict, main: dict) -> None:
    pump_checkbox = find_caption(main, "PumpCheckbox1")
    radio_pic_tab = copy.deepcopy(pump_checkbox["picTab"])
    radio_icon_position = {
        "height": 28,
        "left": 0,
        "top": 38,
        "width": 28,
    }
    ethernet, ethernet_header, _ = decode_ftu(UI_DIR / "ethernetsetting.ftu")
    radio_before = control_signature(ethernet)
    for caption in ("DYNAMIC_IP_BUTTON", "STATIC_IP_BUTTON"):
        button = find_caption(ethernet, caption)
        button["picTab"] = copy.deepcopy(radio_pic_tab)
        button["iconPosition"] = copy.deepcopy(radio_icon_position)
        button["textPosition"] = {
            "height": 104,
            "left": 36,
            "top": 0,
            "width": 158,
        }
    style_ethernet_content(ethernet)
    ethernet_path = UI_DIR / "ethernetsetting.ftu"
    ethernet_path.write_bytes(encode_ftu(ethernet, ethernet_header))
    verified, _, _ = decode_ftu(ethernet_path)
    if sorted(control_signature(verified)) != sorted(radio_before):
        raise RuntimeError("ethernet radio control IDs/captions changed")

    wifi_toggle = find_caption(style, "ButtonOnOff")
    display_path = UI_DIR / "setdisplay.ftu"
    display, display_header, _ = decode_ftu(display_path)
    display_before = control_signature(display)
    display_button = find_caption(display, "SetDisplayTimeButton")
    display_button["picTab"] = copy.deepcopy(wifi_toggle["picTab"])
    display_button["iconPosition"] = {
        "height": 65,
        "left": 0,
        "top": 0,
        "width": 65,
    }
    display_edit = find_caption(display, "SetDisplayTimeEditText")
    display_edit.pop("bgColorTab", None)
    display_edit["backgroundPic"] = EDIT_BACKGROUND_NAME
    display_edit["alignment"] = 37
    display_edit["colorTab"] = {"color0": 23483}
    display_edit["hintTextColor"] = 23483
    display_edit["position"] = {
        "height": 61,
        "left": 405,
        "top": 149,
        "width": 204,
    }
    seekbar = find_caption(display, "SeekBar")
    seekbar["backgroundPic"] = BRIGHTNESS_BACKGROUND_NAME
    seekbar["progressPic"] = BRIGHTNESS_PROGRESS_NAME
    seekbar["thumb"] = {
        "normalPic": BRIGHTNESS_THUMB_NAME,
        "size": {"height": 40, "width": 40},
    }
    seekbar["position"] = {
        "height": 67,
        "left": 280,
        "top": 276,
        "width": 650,
    }
    brightness_label = find_caption(display, "TextView1")
    brightness_label["position"] = {
        "height": 67,
        "left": 60,
        "top": 276,
        "width": 200,
    }
    display_path.write_bytes(encode_ftu(display, display_header))
    verified, _, _ = decode_ftu(display_path)
    if control_signature(verified) != display_before:
        raise RuntimeError("display toggle control IDs/captions changed")


def style_topset_header() -> None:
    path = UI_DIR / "page1topset.ftu"
    data, header, _ = decode_ftu(path)
    before_signature = control_signature(data)
    title = find_caption(data, "TextView7")
    title["backgroundPic"] = TOPSET_GENERAL_HEADER_NAME
    title["text"] = ""
    title["colorTab"] = {"color0": 0x168BFF}
    title["position"] = {
        "height": TOPSET_GENERAL_HEADER_SIZE[1],
        "left": (1024 - TOPSET_GENERAL_HEADER_SIZE[0]) // 2,
        "top": 0,
        "width": TOPSET_GENERAL_HEADER_SIZE[0],
    }
    title.pop("textPosition", None)
    path.write_bytes(encode_ftu(data, header))
    verified, _, _ = decode_ftu(path)
    if control_signature(verified) != before_signature:
        raise RuntimeError("TopSet TextView7 IDs/captions changed")


def main() -> None:
    style, _, _ = decode_ftu(WIFI_UI)
    main_ui, _, _ = decode_ftu(MAIN_UI)
    build_brightness_seekbar_assets()
    build_header_icon_assets()
    build_general_header_icon_asset()
    for filename, config in PAGE_CONFIG.items():
        style_page(UI_DIR / filename, config, style)
        print(f"styled {filename}")
    style_setting_button_assets(style, main_ui)
    style_topset_header()
    print("applied PumpCheckbox1 radio artwork and Wi-Fi toggle artwork")
    print("applied page header icon cards and the TopSet general-settings card")
    print("preserved control IDs/captions for all styled pages")


if __name__ == "__main__":
    main()
