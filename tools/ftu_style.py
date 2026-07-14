#!/usr/bin/env python3
"""Apply the cj96 visual system while preserving FTU control IDs and bindings."""

from __future__ import annotations

import argparse
import io
import json
import math
import shutil
import struct
import zlib
from pathlib import Path

import resvg_py
from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageFont


ROOT = Path(__file__).resolve().parents[1]
UI_DIR = ROOT / "ui"
RESOURCE_DIR = ROOT / "resources"
BACKUP_DIR = ROOT / "Release" / "ui_redesign_backup"
LUCIDE_DIR = ROOT / "Release" / "ui_assets" / "lucide"
VISUAL_DIR = ROOT / "Release" / "ui_assets" / "visuals"


COLORS = {
    "background": 0xF2F5F8,
    "surface": 0xFFFFFF,
    "surface_soft": 0xF3F5F8,
    "border": 0xD9DFE7,
    "control": 0xC7CDD5,
    "ink": 0x1D1D1F,
    "muted": 0x737A84,
    "header": 0xFBFCFE,
    "primary": 0x007AFF,
    "primary_dark": 0x0066CC,
    "primary_soft": 0xE8F2FF,
    "amber": 0xFF9F0A,
    "danger": 0xFF3B30,
    "danger_dark": 0xD92D20,
    "danger_soft": 0xFFF0EF,
    "success": 0x34C759,
    "success_dark": 0x248A3D,
    "success_soft": 0xECF8EF,
}


def rgb(name: str) -> tuple[int, int, int]:
    value = COLORS[name]
    return (value >> 16, (value >> 8) & 0xFF, value & 0xFF)


def render_lucide(name: str, size: int, color: tuple[int, int, int]) -> Image.Image:
    svg = (LUCIDE_DIR / f"{name}.svg").read_text(encoding="utf-8")
    svg = svg.replace("currentColor", "#%02x%02x%02x" % color)
    png = resvg_py.svg_to_bytes(
        svg_string=svg,
        width=size,
        height=size,
        shape_rendering="geometric_precision",
    )
    return Image.open(io.BytesIO(png)).convert("RGBA")


def decode_ftu(path: Path) -> tuple[dict, bytes, int]:
    blob = path.read_bytes()
    if blob[:4] != b"ZKSW" or blob[-4:] != b"ZKSW":
        raise ValueError(f"{path} is not a ZKSW FTU file")
    payload_offset = struct.unpack_from("<I", blob, 6)[0] + 8
    payload = zlib.decompress(blob[payload_offset:-6])
    return json.loads(payload.decode("utf-8")), blob[:payload_offset], payload_offset


def encode_ftu(data: dict, header: bytes) -> bytes:
    payload = json.dumps(
        data, ensure_ascii=False, separators=(",", ":")
    ).encode("utf-8")
    compressed = zlib.compress(payload)

    # Mirrors com.jcraft.jzlib.zkzip bundled with FlyThings IDE 2.x.
    data_start = (len(payload) - len(compressed)) % 30 + 23
    result = bytearray(b"ZKSW\xAA\x55")
    result.extend(struct.pack("<I", data_start - 8))
    result.extend(struct.pack("<I", len(payload) + 8))
    result.extend(struct.pack("<I", len(compressed) + 16))
    for i in range(data_start - 18):
        index = (((i + 2) * 3) // 2) + 4
        if index >= len(compressed):
            index = 0
        result.append(compressed[index])
    result.extend(compressed)
    result.extend(b"\x55\xAAZKSW")
    return bytes(result)


def contains_back_button(node: dict) -> bool:
    return any(
        isinstance(value, dict) and value.get("caption") == "sys_back"
        for value in node.values()
    )


def is_header_window(node: dict) -> bool:
    position = node.get("position", {})
    return (
        contains_back_button(node)
        and position.get("top", 999) <= 20
        and position.get("height", 999) <= 110
    )


def set_tab_colors(tab: dict | None, colors: list[int]) -> None:
    if tab is None:
        return
    keys = sorted(tab)
    for index, key in enumerate(keys):
        tab[key] = colors[min(index, len(colors) - 1)]


def style_list_item(node: dict, caption: str) -> None:
    is_header = "Tip" in caption or caption.endswith("HeaderSubItem")
    background = COLORS["surface_soft"] if is_header else COLORS["surface"]
    node["backgroundColor"] = background
    if "bgColorTab" in node:
        set_tab_colors(
            node["bgColorTab"],
            [background, COLORS["primary_soft"], COLORS["primary_soft"]],
        )
    if "colorTab" in node:
        set_tab_colors(node["colorTab"], [COLORS["ink"], COLORS["ink"]])
    if is_header:
        node["bold"] = True


def style_button(node: dict, page_name: str, in_header: bool) -> None:
    caption = node.get("caption", "")
    text = node.get("text", "")
    pictures = node.get("picTab", {})

    if page_name == "UserIme":
        node["bgColorTab"] = {
            "color0": COLORS["surface"],
            "color1": COLORS["primary_soft"],
            "color2": COLORS["primary_soft"],
            "color3": COLORS["surface_soft"],
            "color4": COLORS["primary_soft"],
        }
        node["colorTab"] = {
            "color0": COLORS["ink"],
            "color1": COLORS["ink"],
            "color2": COLORS["primary_dark"],
            "color3": COLORS["muted"],
            "color4": COLORS["primary_dark"],
        }
        height = node.get("position", {}).get("height", 48)
        node["fontSize"] = min(32, max(20, height // 2))
        return

    if caption == "screenshotButton":
        node["text"] = "截图"
        node["fontSize"] = 20
        node["bold"] = True
        node["bgColorTab"] = {
            "color0": COLORS["primary_soft"],
            "color1": COLORS["primary_soft"],
        }
        node["colorTab"] = {
            "color0": COLORS["primary"],
            "color1": COLORS["primary_dark"],
        }
        return

    if caption == "Button9" and page_name == "main":
        node["bgColorTab"] = {"color0": COLORS["header"]}
        node["colorTab"] = {"color0": COLORS["ink"]}
        node["fontSize"] = 28
        node["bold"] = True
        return

    if in_header:
        if "colorTab" in node:
            set_tab_colors(node["colorTab"], [COLORS["primary"]])
        return

    if pictures:
        if any(name.startswith("bgr_btn") for name in pictures.values()):
            return
        if any(
            token in name
            for name in pictures.values()
            for token in ("toggle-", "radio_", "check_", "btn_check", "sysbar_back")
        ):
            return

    if page_name == "page1topset" and caption in {
        "OpenWifiButton",
        "OpenNetButton",
        "Open4GButton",
        "SetSysTimeBtn",
        "DisplayBtn",
        "LanBtn",
    }:
        node.pop("bgColorTab", None)
        asset = {
            "OpenWifiButton": "setting_wifi",
            "OpenNetButton": "setting_ethernet",
            "Open4GButton": "setting_cellular",
            "SetSysTimeBtn": "setting_time",
            "DisplayBtn": "setting_display",
            "LanBtn": "setting_language",
        }[caption]
        node["picTab"] = {
            "pic0": f"{asset}.png",
            "pic1": f"{asset}_pressed.png",
        }
        node["colorTab"] = {
            "color0": COLORS["ink"],
            "color1": COLORS["primary_dark"],
        }
        node["fontSize"] = 28
        node["bold"] = True
        return

    lowered = f"{caption} {text}".lower()
    label_texts = {
        "开始时间", "喷雾程序", "星期模式", "隔天模式", "解码器类型",
        "灌溉时间", "浸泡时间", "喷雾间隔", "设置运行时间",
        "间隔", "延后", "时", "分", "秒", "天", "~",
    }
    section_headers = {"Button20", "Button21", "Button22", "Button23"}
    selectable = caption in {
        "WeekModeButton", "IntervalModeButton", "WednesdayButton",
        "ThursdayButton", "FridayButton", "SaturdayButton", "EverDayButton",
        "SundayButton", "MondayButton", "TuesdayButton",
    }

    if caption == "UartValueOnButton":
        backgrounds = [COLORS["success"], COLORS["success_dark"]]
        foregrounds = [COLORS["surface"], COLORS["surface"]]
    elif caption == "UartValueOffButton":
        backgrounds = [COLORS["danger"], COLORS["danger_dark"]]
        foregrounds = [COLORS["surface"], COLORS["surface"]]
    elif any(token in lowered for token in ("del", "delete", "删除", "清空")):
        backgrounds = [COLORS["danger_soft"], COLORS["danger"]]
        foregrounds = [COLORS["danger"], COLORS["surface"]]
    elif any(token in lowered for token in ("cancel", "cencel", "取消", "返回")):
        backgrounds = [COLORS["surface_soft"], COLORS["border"]]
        foregrounds = [COLORS["primary"], COLORS["primary_dark"]]
    elif text in label_texts or caption in section_headers:
        backgrounds = [COLORS["surface_soft"], COLORS["surface_soft"]]
        foregrounds = [COLORS["ink"], COLORS["ink"]]
        node["bold"] = True
    elif selectable:
        backgrounds = [COLORS["surface"], COLORS["primary_soft"], COLORS["primary"]]
        foregrounds = [COLORS["ink"], COLORS["primary"], COLORS["surface"]]
    else:
        backgrounds = [COLORS["primary"], COLORS["primary_dark"]]
        foregrounds = [COLORS["surface"], COLORS["surface"]]

    if selectable and pictures:
        node["colorTab"] = {
            "color0": COLORS["ink"],
            "color1": COLORS["ink"],
            "color2": COLORS["ink"],
        }
        return

    if "bgColorTab" in node or not pictures:
        node["bgColorTab"] = {
            f"color{index}": color for index, color in enumerate(backgrounds)
        }
        node["colorTab"] = {
            f"color{index}": color for index, color in enumerate(foregrounds)
        }
        height = node.get("position", {}).get("height", 48)
        node["fontSize"] = min(24, max(17, height // 2))


def style_text(node: dict, in_header: bool) -> None:
    caption = node.get("caption", "")
    background_tab = node.get("bgColorTab")

    if in_header:
        node["colorTab"] = {"color0": COLORS["ink"]}
        return

    if caption == "SysRunShowTextView":
        node["bgColorTab"] = {"color0": COLORS["success_soft"]}
        node["colorTab"] = {"color0": COLORS["success_dark"]}
        node["bold"] = True
        return
    if caption in {"SysRunTextView", "WaterBarTextView", "TrafficTextView"}:
        node["bgColorTab"] = {"color0": COLORS["surface_soft"]}
        node["colorTab"] = {"color0": COLORS["muted"]}
        node["bold"] = True
        return
    if caption in {"WaterSourceTextView", "WaterSource1TextView"}:
        node["bgColorTab"] = {"color0": COLORS["primary_soft"]}
        node["colorTab"] = {"color0": COLORS["primary"]}
        node["bold"] = True
        return
    if caption.startswith("PumpTextView"):
        node["bgColorTab"] = {"color0": COLORS["surface"]}
        node["colorTab"] = {"color0": COLORS["ink"]}
        node["bold"] = True
        return

    if background_tab:
        current = background_tab.get("color0", COLORS["surface"])
        if current in (0x0000FF, 0x00BDD5, 0x5578FC):
            background = COLORS["primary_soft"]
        elif current in (0x9F9F9F, 0xA0A0A0):
            background = COLORS["surface_soft"]
        elif current in (0xC0C0C0, 0xE0E0E0):
            background = COLORS["surface_soft"]
        elif current == 0:
            background = COLORS["surface_soft"]
        else:
            background = COLORS["surface"]
        set_tab_colors(background_tab, [background])
        foreground = (
            COLORS["surface"]
            if background == COLORS["danger"]
            else COLORS["ink"]
        )
        node["colorTab"] = {"color0": foreground}
    else:
        node["colorTab"] = {"color0": COLORS["ink"]}

    if caption.endswith("ShowTextView"):
        node["bold"] = True


def style_control(
    node: dict,
    kind: str,
    page_name: str,
    in_header: bool,
    caption: str,
) -> None:
    if kind == "window":
        if is_header_window(node):
            if "backgroundColor" in node:
                node["backgroundColor"] = COLORS["header"]
        elif "backgroundColor" in node:
            node["backgroundColor"] = COLORS["surface"]
        return

    if kind == "button":
        style_button(node, page_name, in_header)
        return

    if kind == "textview":
        style_text(node, in_header)
        return

    if kind == "edittext":
        node["bgColorTab"] = {"color0": COLORS["surface_soft"]}
        node.pop("backgroundColor", None)
        node["colorTab"] = {"color0": COLORS["ink"]}
        node["hintTextColor"] = COLORS["muted"]
        return

    if kind == "radiobuttons":
        node["bgColorTab"] = {
            "color0": COLORS["surface_soft"],
            "color2": COLORS["primary"],
        }
        node["colorTab"] = {
            "color0": COLORS["ink"],
            "color2": COLORS["surface"],
        }
        return

    if kind == "digitalclock":
        node["clockColor"] = COLORS["ink"]
        return

    if kind in ("item", "subitem"):
        style_list_item(node, caption)


def walk_controls(
    node: dict,
    page_name: str,
    inherited_header: bool = False,
    kind: str = "root",
) -> None:
    current_header = inherited_header or (kind == "window" and is_header_window(node))
    caption = node.get("caption", "")
    style_control(node, kind, page_name, current_header, caption)

    for key, value in list(node.items()):
        if isinstance(value, dict):
            child_kind = key.split("__", 1)[0] if "__" in key else key.lower()
            if child_kind == "subitem":
                child_kind = "subitem"
            walk_controls(value, page_name, current_header, child_kind)
        elif isinstance(value, list):
            child_kind = "subitem" if key == "subItem" else key.lower()
            for child in value:
                if isinstance(child, dict):
                    walk_controls(child, page_name, current_header, child_kind)


def layout_settings_home(data: dict) -> None:
    layout = {
        "OpenWifiButton": (58, 145),
        "OpenNetButton": (372, 145),
        "Open4GButton": (686, 145),
        "SetSysTimeBtn": (58, 270),
        "DisplayBtn": (372, 270),
        "LanBtn": (686, 270),
    }
    for value in data.values():
        if not isinstance(value, dict):
            continue
        caption = value.get("caption")
        if caption in layout:
            left, top = layout[caption]
            value["position"] = {
                "height": 82,
                "left": left,
                "top": top,
                "width": 280,
            }
            value["iconPosition"] = {
                "height": 82,
                "left": 0,
                "top": 0,
                "width": 280,
            }
        elif caption in ("Button2", "Button3") and not value.get("text"):
            value["visible"] = False


def layout_display_settings(data: dict) -> None:
    positions = {
        "TextTime": (170, 155, 220, 60),
        "TextView1": (170, 290, 220, 60),
        "SetDisplayTimeEditText": (405, 145, 220, 70),
        "SetDisplayTimeButton": (665, 148, 120, 65),
        "SeekBar": (430, 286, 350, 67),
    }
    for value in data.values():
        if not isinstance(value, dict):
            continue
        caption = value.get("caption")
        if caption in positions:
            left, top, width, height = positions[caption]
            value["position"] = {
                "height": height,
                "left": left,
                "top": top,
                "width": width,
            }
        if caption in ("TextTime", "TextView1"):
            value["fontSize"] = 36


def layout_secondary_headers(node: dict) -> None:
    for key, value in node.items():
        if not isinstance(value, dict):
            continue
        kind = key.split("__", 1)[0] if "__" in key else key.lower()
        if kind == "window" and is_header_window(value):
            for child_key, child in value.items():
                if not isinstance(child, dict):
                    continue
                child_kind = child_key.split("__", 1)[0] if "__" in child_key else child_key.lower()
                caption = child.get("caption", "")
                if caption == "sys_back":
                    child["position"] = {
                        "height": 88,
                        "left": 0,
                        "top": 0,
                        "width": 88,
                    }
                elif child_kind == "textview":
                    child["position"] = {
                        "height": 56,
                        "left": 337,
                        "top": 8,
                        "width": 350,
                    }
                    child["fontSize"] = 34
                    child["bold"] = True
                    child["colorTab"] = {"color0": COLORS["ink"]}
        else:
            layout_secondary_headers(value)


def layout_main(data: dict) -> None:
    positions = {
        1: 35,
        2: 230,
        3: 425,
        4: 620,
        5: 815,
    }
    overview_text_positions = {
        "SysRunTextView": (46, 22, 54, 28, 15, COLORS["muted"], COLORS["surface_soft"], "状态"),
        "SysRunShowTextView": (98, 22, 86, 28, 20, COLORS["success_dark"], COLORS["success_soft"], None),
        "WaterBarTextView": (210, 22, 54, 28, 15, COLORS["muted"], COLORS["surface_soft"], "水压"),
        "WaterBarShowTextView": (262, 22, 78, 28, 19, COLORS["ink"], COLORS["surface"], None),
        "TrafficTextView": (368, 22, 54, 28, 15, COLORS["muted"], COLORS["surface_soft"], "流量"),
        "TrafficShowTextView": (420, 22, 82, 28, 19, COLORS["ink"], COLORS["surface"], None),
    }
    overview_hidden = {
        "WaterSourceTextView",
        "WaterSource1TextView",
        "PumpCheckbox1",
        "PumpCheckbox2",
        "PumpCheckbox3",
        "PumpCheckbox4",
        "PumpCheckbox5",
        "PumpTextView1",
        "PumpTextView2",
        "PumpTextView3",
        "PumpTextView4",
        "PumpTextView5",
        "WaterBarPicTextView1",
        "WaterBarPicTextView2",
        "WaterBar",
        "TextView2",
    }

    def remove_water_source_anim_children(node: dict) -> None:
        for key, value in list(node.items()):
            if isinstance(value, dict):
                if value.get("caption") == "WaterSourceImageAnim":
                    del node[key]
                else:
                    remove_water_source_anim_children(value)
            elif isinstance(value, list):
                for item in value:
                    if isinstance(item, dict):
                        remove_water_source_anim_children(item)

    def layout_overview_children(node: dict) -> None:
        caption = node.get("caption")
        if caption in overview_text_positions:
            left, top, width, height, font_size, color, background, text = overview_text_positions[caption]
            node["position"] = {
                "height": height,
                "left": left,
                "top": top,
                "width": width,
            }
            node["visible"] = True
            node["fontSize"] = font_size
            node["bold"] = True
            node["colorTab"] = {"color0": color}
            node["bgColorTab"] = {"color0": background}
            if text is not None:
                node["text"] = text
        elif caption in overview_hidden:
            node["visible"] = False

        for child in node.values():
            if isinstance(child, dict):
                layout_overview_children(child)
            elif isinstance(child, list):
                for item in child:
                    if isinstance(item, dict):
                        layout_overview_children(item)

    for value in data.values():
        if not isinstance(value, dict):
            continue
        caption = value.get("caption")
        if caption == "Window1":
            value["backgroundPic"] = "overview_panel.jpg"
            layout_overview_children(value)
            remove_water_source_anim_children(value)
        elif caption == "Window3":
            value["backgroundPic"] = "bgr_mid.png"

        if caption == "Button9":
            value["position"] = {
                "height": 56,
                "left": 312,
                "top": 14,
                "width": 400,
            }
        elif caption == "screenshotButton":
            value["position"] = {
                "height": 48,
                "left": 24,
                "top": 18,
                "width": 120,
            }
        elif caption == "DigitalClock1":
            value["position"] = {
                "height": 48,
                "left": 846,
                "top": 18,
                "width": 150,
            }
        if caption and caption.startswith("Button") and caption[6:].isdigit():
            index = int(caption[6:])
            if index in positions:
                value["visible"] = True
                value["position"] = {
                    "height": 72,
                    "left": positions[index],
                    "top": 520,
                    "width": 174,
                }
                value["iconPosition"] = {
                    "height": 72,
                    "left": 2,
                    "top": 0,
                    "width": 170,
                }
            elif 6 <= index <= 8:
                value["visible"] = False


def apply_style(path: Path) -> None:
    data, header, _ = decode_ftu(path)
    page_name = path.stem

    if "backgroundPic" in data:
        data["backgroundPic"] = "background.png"
    if "backgroundColor" in data:
        data["backgroundColor"] = COLORS["background"]

    walk_controls(data, page_name)

    if page_name not in {"main", "UserIme"}:
        layout_secondary_headers(data)

    if page_name == "page1topset":
        layout_settings_home(data)
    elif page_name == "setdisplay":
        layout_display_settings(data)
    elif page_name == "main":
        layout_main(data)

    encoded = encode_ftu(data, header)
    path.write_bytes(encoded)
    decoded, _, _ = decode_ftu(path)
    if decoded != data:
        raise RuntimeError(f"round-trip verification failed for {path}")


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    fonts = Path(r"C:\Windows\Fonts")
    name = "msyhbd.ttc" if bold else "msyh.ttc"
    return ImageFont.truetype(str(fonts / name), size=size)


def draw_centered_text(
    draw: ImageDraw.ImageDraw,
    box: tuple[int, int, int, int],
    text: str,
    text_font: ImageFont.FreeTypeFont,
    fill: tuple[int, int, int],
) -> None:
    bounds = draw.textbbox((0, 0), text, font=text_font)
    width = bounds[2] - bounds[0]
    height = bounds[3] - bounds[1]
    x = box[0] + (box[2] - box[0] - width) / 2
    y = box[1] + (box[3] - box[1] - height) / 2 - bounds[1]
    draw.text((x, y), text, font=text_font, fill=fill)


def vertical_gradient(
    size: tuple[int, int],
    top: tuple[int, int, int],
    bottom: tuple[int, int, int],
) -> Image.Image:
    image = Image.new("RGB", size, top)
    draw = ImageDraw.Draw(image)
    denominator = max(1, size[1] - 1)
    for y in range(size[1]):
        ratio = y / denominator
        color = tuple(
            round(top[channel] * (1 - ratio) + bottom[channel] * ratio)
            for channel in range(3)
        )
        draw.line((0, y, size[0], y), fill=color)
    return image


def make_tech_background() -> Image.Image:
    image = vertical_gradient((1024, 600), (249, 250, 252), (239, 243, 247))
    draw = ImageDraw.Draw(image)
    draw.rectangle((0, 0, 1023, 87), fill=rgb("header"))
    draw.line((0, 87, 1023, 87), fill=rgb("border"), width=1)
    draw.line((0, 88, 1023, 88), fill=(232, 236, 241), width=1)
    return image


def make_panel() -> Image.Image:
    image = make_tech_background().crop((0, 89, 1007, 524)).convert("RGBA")
    shadow = Image.new("RGBA", image.size, (0, 0, 0, 0))
    ImageDraw.Draw(shadow).rounded_rectangle(
        (7, 8, 999, 429),
        8,
        fill=(72, 84, 99, 34),
    )
    image = Image.alpha_composite(image, shadow.filter(ImageFilter.GaussianBlur(6)))

    panel = vertical_gradient((999, 425), (255, 255, 255), (250, 251, 253))
    mask = Image.new("L", panel.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, 998, 424), 8, fill=255)
    image.paste(panel, (4, 3), mask)
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((3, 2, 1003, 430), 8, outline=rgb("border"), width=1)
    return image.convert("RGB")


def crop_fill(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    source_ratio = image.width / image.height
    target_ratio = size[0] / size[1]
    if source_ratio > target_ratio:
        width = round(image.height * target_ratio)
        left = (image.width - width) // 2
        image = image.crop((left, 0, left + width, image.height))
    else:
        height = round(image.width / target_ratio)
        top = (image.height - height) // 2
        image = image.crop((0, top, image.width, top + height))
    return image.resize(size, Image.Resampling.LANCZOS)


def make_overview_panel() -> Image.Image:
    image = make_panel().convert("RGBA")
    draw = ImageDraw.Draw(image)

    def soft_card(box: tuple[int, int, int, int]) -> None:
        draw.rounded_rectangle(
            box,
            8,
            fill=rgb("surface_soft") + (255,),
            outline=rgb("border") + (255,),
            width=1,
        )

    def metric_slot(box: tuple[int, int, int, int], fill: tuple[int, int, int]) -> None:
        draw.rounded_rectangle(box, 8, fill=fill + (255,), outline=rgb("border") + (255,), width=1)

    def icon_circle(center: tuple[int, int], radius: int, color: tuple[int, int, int]) -> None:
        x, y = center
        draw.ellipse(
            (x - radius, y - radius, x + radius, y + radius),
            fill=(255, 255, 255, 238),
            outline=color + (255,),
            width=2,
        )

    def centered_lines(
        center: tuple[int, int],
        lines: tuple[str, ...],
        text_font: ImageFont.FreeTypeFont,
        fill: tuple[int, int, int],
        line_gap: int = 3,
    ) -> None:
        boxes = [draw.textbbox((0, 0), line, font=text_font) for line in lines]
        heights = [box[3] - box[1] for box in boxes]
        total_height = sum(heights) + line_gap * (len(lines) - 1)
        y = center[1] - total_height / 2
        for line, box, height in zip(lines, boxes, heights):
            width = box[2] - box[0]
            draw.text((center[0] - width / 2, y - box[1]), line, font=text_font, fill=fill)
            y += height + line_gap

    def pipe(points: tuple[tuple[int, int], ...], color: tuple[int, int, int]) -> None:
        for start, end in zip(points, points[1:]):
            draw.line((*start, *end), fill=(173, 220, 255, 255), width=16)
            draw.line((*start, *end), fill=(245, 251, 255, 255), width=10)
            draw.line((*start, *end), fill=color + (255,), width=5)

    def arrow_tip(x: int, y: int) -> None:
        draw.polygon(((x, y), (x - 9, y - 6), (x - 9, y + 6)), fill=rgb("primary") + (255,))

    def small_label(box: tuple[int, int, int, int], text: str, color: tuple[int, int, int]) -> None:
        draw.rounded_rectangle(box, 7, fill=(255, 255, 255, 222), outline=color + (180,), width=1)
        draw_centered_text(draw, box, text, font(15, True), color)

    primary = rgb("primary")
    flow_card = (20, 18, 986, 376)
    soft_card(flow_card)

    metric_slots = (
        (34, 18, 190, 54),
        (198, 18, 346, 54),
        (356, 18, 508, 54),
        (526, 18, 960, 54),
    )
    for box in metric_slots:
        metric_slot(box, rgb("surface_soft"))
    for x, y, icon_color, icon_kind in (
        (45, 36, rgb("success_dark"), "status"),
        (210, 36, primary, "pressure"),
        (368, 36, primary, "flow"),
    ):
        if icon_kind == "status":
            draw.ellipse((x - 7, y - 7, x + 7, y + 7), fill=icon_color + (255,))
            draw.line((x - 3, y, x, y + 4, x + 6, y - 5), fill=(255, 255, 255, 255), width=2)
        elif icon_kind == "pressure":
            draw.arc((x - 8, y - 8, x + 8, y + 8), 200, 340, fill=icon_color + (255,), width=2)
            draw.line((x, y, x + 7, y - 6), fill=rgb("ink") + (255,), width=2)
        else:
            draw.line((x - 8, y, x + 8, y), fill=icon_color + (255,), width=2)
            draw.line((x, y - 8, x, y + 8), fill=icon_color + (255,), width=2)
            draw.ellipse((x - 4, y - 4, x + 4, y + 4), fill=icon_color + (255,))

    draw.text((540, 29), "蓝=当前，两侧=最近", font=font(12, True), fill=rgb("ink"))
    top_legend_items = (
        (680, 36, rgb("success"), "已完成"),
        (760, 36, primary, "正在运行"),
        (858, 36, rgb("amber"), "即将运行"),
    )
    for x, y, color, text in top_legend_items:
        draw.ellipse((x - 6, y - 6, x + 6, y + 6), fill=color + (255,), outline=(255, 255, 255, 255), width=1)
        draw.text((x + 12, y - 8), text, font=font(11, True), fill=rgb("ink"))

    draw.text((40, 72), "水路总览", font=font(26, True), fill=rgb("ink"))
    draw.text((160, 80), "水源 → 流量表 → 水压表 → 阀门组开阀", font=font(17, False), fill=rgb("muted"))

    # Main water path. The water source itself is an overlaid animated control.
    pipe(((182, 224), (230, 224), (314, 224), (408, 224)), primary)
    for x in (226, 310, 404):
        arrow_tip(x, 224)
    for x in (214, 284, 372):
        draw.ellipse((x - 4, 220, x + 4, 228), fill=primary + (210,))

    # Flow meter.
    icon_circle((260, 224), 34, primary)
    draw.ellipse((239, 203, 281, 245), fill=rgb("primary_soft") + (255,), outline=primary + (255,), width=2)
    draw.line((260, 224, 280, 224), fill=primary + (255,), width=3)
    draw.line((260, 224, 247, 208), fill=primary + (255,), width=3)
    draw.line((260, 224, 247, 240), fill=primary + (255,), width=3)
    draw.ellipse((254, 218, 266, 230), fill=primary + (255,))
    small_label((218, 270, 302, 298), "流量表", primary)

    # Pressure meter.
    icon_circle((356, 224), 34, primary)
    draw.arc((335, 203, 377, 245), 200, 340, fill=primary + (255,), width=5)
    draw.line((356, 224, 371, 211), fill=rgb("ink") + (255,), width=3)
    draw.ellipse((350, 218, 362, 230), fill=rgb("ink") + (255,))
    small_label((314, 270, 398, 298), "水压表", primary)

    # Centered valve group.
    manifold = (408, 128, 604, 322)
    shadow = Image.new("RGBA", image.size, (0, 0, 0, 0))
    ImageDraw.Draw(shadow).rounded_rectangle((414, 136, 610, 326), 8, fill=(72, 84, 99, 26))
    image.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(5)))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(manifold, 8, fill=(255, 255, 255, 235), outline=rgb("border") + (255,), width=1)
    draw_centered_text(draw, (408, 140, 604, 168), "阀门组", font(23, True), rgb("ink"))
    draw.line((430, 224, 582, 224), fill=(173, 220, 255, 255), width=12)
    draw.line((430, 224, 582, 224), fill=primary + (255,), width=6)
    pipe(((414, 224), (430, 224)), primary)
    state_nodes = (
        (456, 224, rgb("success"), "已完成", "上一组", "前", (424, 268, 488, 312)),
        (506, 224, primary, "正在运行", "当前组", "中", (474, 268, 538, 312)),
        (556, 224, rgb("amber"), "即将运行", "下一组", "后", (524, 268, 588, 312)),
    )
    for x, y, color, title, name, node_text, label_box in state_nodes:
        radius = 25 if title == "正在运行" else 19
        draw.line((x, y, x, y + 39), fill=color + (255,), width=4)
        if title == "正在运行":
            draw.ellipse((x - 35, y - 35, x + 35, y + 35), outline=color + (72,), width=5)
            draw.ellipse((x - 29, y - 29, x + 29, y + 29), outline=color + (150,), width=3)
        draw.ellipse((x - radius, y - radius, x + radius, y + radius), fill=color + (255,), outline=(255, 255, 255, 255), width=3)
        draw_centered_text(draw, (x - radius, y - radius, x + radius, y + radius), node_text, font(15, True), rgb("surface"))
        draw.rounded_rectangle(label_box, 6, fill=(255, 255, 255, 244), outline=color + (190,), width=1)
        centered_lines(
            ((label_box[0] + label_box[2]) // 2, (label_box[1] + label_box[3]) // 2),
            (title, name),
            font(12, True),
            color,
            1,
        )

    # Branches to the field and small output icons.
    for x, color in ((456, rgb("success")), (506, primary), (556, rgb("amber"))):
        draw.line((x, 224, x, 338), fill=color + (190,), width=3)
        draw.ellipse((x - 8, 330, x + 8, 346), fill=color + (230,), outline=(255, 255, 255, 255), width=2)
        for dx in (-16, 0, 16):
            draw.arc((x + dx - 14, 346, x + dx + 14, 368), 200, 340, fill=color + (150,), width=1)
    draw.rounded_rectangle((632, 126, 960, 198), 8, fill=(255, 255, 255, 210), outline=rgb("border") + (255,), width=1)
    draw_centered_text(draw, (648, 136, 934, 162), "蓝色为当前开阀组，两侧为最近阀门组", font(16, True), rgb("ink"))
    legend_items = (
        (662, 180, rgb("success"), "已完成"),
        (778, 180, primary, "正在运行"),
        (914, 180, rgb("amber"), "即将运行"),
    )
    for x, y, color, text in legend_items:
        draw.ellipse((x - 9, y - 9, x + 9, y + 9), fill=color + (255,), outline=(255, 255, 255, 255), width=2)
        draw.text((x + 16, y - 12), text, font=font(15, True), fill=rgb("ink"))

    # Small device pictograms on the right make the system feel less empty.
    for x, label, icon in ((700, "传感器", "sensor"), (812, "管路", "pipe"), (918, "喷头", "spray")):
        draw.rounded_rectangle((x - 34, 226, x + 34, 292), 8, fill=(255, 255, 255, 218), outline=rgb("border") + (255,), width=1)
        if icon == "sensor":
            draw.rounded_rectangle((x - 13, 240, x + 13, 265), 5, fill=rgb("primary_soft") + (255,), outline=primary + (255,), width=2)
            draw.line((x, 265, x, 274), fill=primary + (255,), width=2)
            draw.arc((x - 20, 232, x + 20, 272), 210, 330, fill=primary + (160,), width=2)
        elif icon == "pipe":
            draw.line((x - 22, 253, x + 22, 253), fill=primary + (255,), width=8)
            draw.line((x, 253, x, 274), fill=primary + (255,), width=8)
            draw.ellipse((x - 7, 246, x + 7, 260), fill=(255, 255, 255, 255), outline=primary + (255,), width=2)
        else:
            draw.line((x, 242, x, 272), fill=primary + (255,), width=4)
            for dx in (-20, -10, 10, 20):
                draw.arc((x + dx - 12, 248, x + dx + 12, 276), 200, 340, fill=primary + (170,), width=2)
        draw_centered_text(draw, (x - 34, 294, x + 34, 318), label, font(14, True), rgb("muted"))

    draw.rectangle((620, 100, 1000, 392), fill=rgb("surface_soft") + (255,))
    draw.line((46, 334, 590, 334), fill=rgb("border") + (255,), width=1)
    draw_centered_text(draw, (74, 344, 180, 366), "动态水源", font(15, True), primary)
    draw_centered_text(draw, (242, 344, 326, 366), "计量", font(15, True), primary)
    draw_centered_text(draw, (352, 344, 436, 366), "监测", font(15, True), primary)
    draw_centered_text(draw, (456, 344, 556, 366), "阀门中心", font(15, True), primary)
    return image.convert("RGB")


def draw_nav_icon(
    draw: ImageDraw.ImageDraw,
    index: int,
    color: tuple[int, int, int],
) -> None:
    line = 2
    if index == 1:
        for x, y in ((30, 9), (49, 9), (30, 25), (49, 25)):
            draw.rounded_rectangle((x, y, x + 12, y + 10), 2, outline=color, width=line)
    elif index == 2:
        draw.line((32, 29, 47, 13, 62, 29), fill=color, width=line)
        for x, y in ((32, 29), (47, 13), (62, 29)):
            draw.ellipse((x - 4, y - 4, x + 4, y + 4), outline=color, width=line)
    elif index == 3:
        draw.rounded_rectangle((31, 10, 63, 34), 3, outline=color, width=line)
        draw.line((31, 17, 63, 17), fill=color, width=line)
        draw.line((39, 7, 39, 14), fill=color, width=line)
        draw.line((55, 7, 55, 14), fill=color, width=line)
    elif index == 4:
        draw.ellipse((33, 8, 61, 36), outline=color, width=line)
        draw.line((39, 22, 45, 28, 56, 16), fill=color, width=3)
    elif index == 5:
        draw.arc((31, 8, 63, 36), 30, 190, fill=color, width=line)
        draw.arc((31, 8, 63, 36), 210, 10, fill=color, width=line)
        draw.polygon(((31, 24), (27, 18), (35, 18)), fill=color)
        draw.polygon(((63, 20), (67, 26), (59, 26)), fill=color)
    elif index == 6:
        for x, top in ((34, 23), (45, 15), (56, 9)):
            draw.rounded_rectangle((x, top, x + 7, 35), 1, fill=color)
    elif index == 7:
        draw.rounded_rectangle((35, 7, 59, 36), 2, outline=color, width=line)
        draw.line((41, 16, 54, 16), fill=color, width=line)
        draw.line((41, 23, 54, 23), fill=color, width=line)
        draw.line((41, 30, 51, 30), fill=color, width=line)
    else:
        draw.arc((35, 9, 59, 34), 190, 350, fill=color, width=line)
        draw.line((35, 26, 35, 31, 59, 31, 59, 26), fill=color, width=line)
        draw.ellipse((45, 34, 49, 38), fill=color)


def make_nav_button(index: int, selected: bool) -> Image.Image:
    image = Image.new("RGB", (170, 72), rgb("background"))
    draw = ImageDraw.Draw(image)
    if selected:
        panel = rgb("primary_soft")
        foreground = rgb("primary")
        draw.rounded_rectangle(
            (2, 2, 167, 69),
            8,
            fill=panel,
            outline=rgb("primary_dark"),
            width=1,
        )
    else:
        foreground = rgb("muted")
    icon_names = (
        "layout-dashboard",
        "network",
        "calendar-days",
        "circle-check-big",
        "refresh-cw",
    )
    if index <= len(icon_names):
        icon = render_lucide(icon_names[index - 1], 30, foreground)
        image.paste(icon, (70, 7), icon)
    else:
        icon_layer = Image.new("RGBA", (94, 60), (0, 0, 0, 0))
        draw_nav_icon(ImageDraw.Draw(icon_layer), index, foreground)
        image.paste(icon_layer, (38, 0), icon_layer)
    labels = ("总览", "设备", "计划", "测试", "调试", "数据", "记录", "告警")
    draw_centered_text(
        draw,
        (2, 45, 168, 68),
        labels[index - 1],
        font(15, selected),
        foreground,
    )
    if selected:
        draw.rounded_rectangle((67, 67, 103, 69), 1, fill=rgb("primary"))
    return image


def make_nine_patch(normal: bool) -> Image.Image:
    image = Image.new("RGBA", (30, 30), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    fill = rgb("surface") if normal else rgb("primary_soft")
    border = rgb("border") if normal else rgb("primary")
    draw.rounded_rectangle((1, 1, 28, 28), 6, fill=fill + (255,), outline=border + (255,), width=1)
    draw.line((8, 0, 21, 0), fill=(0, 0, 0, 255), width=1)
    draw.line((0, 8, 0, 21), fill=(0, 0, 0, 255), width=1)
    draw.line((8, 29, 21, 29), fill=(0, 0, 0, 255), width=1)
    draw.line((29, 8, 29, 21), fill=(0, 0, 0, 255), width=1)
    return image


def make_toggle(enabled: bool, invalid: bool = False) -> Image.Image:
    image = Image.new("RGBA", (50, 50), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    track = rgb("control") if not enabled else rgb("success")
    if invalid:
        track = (181, 190, 193)
    draw.rounded_rectangle((3, 13, 47, 37), 12, fill=track + (255,))
    knob_x = 35 if enabled else 15
    draw.ellipse((knob_x - 9, 16, knob_x + 9, 34), fill=rgb("surface") + (255,))
    return image


def make_checkbox(checked: bool, size: int) -> Image.Image:
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    inset = max(1, size // 16)
    draw.rounded_rectangle(
        (inset, inset, size - inset - 1, size - inset - 1),
        max(2, size // 6),
        fill=(rgb("primary") if checked else rgb("surface")) + (255,),
        outline=(rgb("primary") if checked else rgb("border")) + (255,),
        width=max(1, size // 12),
    )
    if checked:
        draw.line(
            (
                size * 0.24,
                size * 0.52,
                size * 0.43,
                size * 0.70,
                size * 0.77,
                size * 0.32,
            ),
            fill=rgb("surface") + (255,),
            width=max(2, size // 10),
            joint="curve",
        )
    return image


def make_radio(checked: bool) -> Image.Image:
    image = Image.new("RGBA", (20, 20), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.ellipse((1, 1, 18, 18), fill=rgb("surface") + (255,), outline=rgb("border") + (255,), width=2)
    if checked:
        draw.ellipse((5, 5, 14, 14), fill=rgb("primary") + (255,))
    return image


def make_water_source_frame(frame_index: int) -> Image.Image:
    primary = rgb("primary")
    pale = (230, 244, 255)
    image = Image.new("RGB", (128, 128), rgb("surface"))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (2, 2, 125, 125),
        8,
        fill=rgb("primary_soft"),
        outline=primary,
        width=2,
    )
    draw.ellipse((28, 20, 98, 90), fill=(255, 255, 255), outline=primary, width=2)
    phase = frame_index / 12.0
    for index, base_y in enumerate((52, 63, 74)):
        points: list[tuple[float, float]] = []
        for x in range(36, 91, 4):
            y = base_y + 5 * math.sin((x / 11.0) + phase * math.tau + index * 0.9)
            points.append((x, y))
        draw.line(points, fill=primary, width=3)
    drop_y = 30 + (frame_index % 6) * 5
    draw.ellipse((59, drop_y, 67, drop_y + 12), fill=(94, 169, 255))
    draw.polygon(((63, drop_y - 7), (57, drop_y + 3), (69, drop_y + 3)), fill=(94, 169, 255))
    draw.arc((36, 58, 90, 86), 15 + frame_index * 8, 165 + frame_index * 8, fill=pale, width=2)
    draw.arc((36, 66, 90, 94), 195 + frame_index * 8, 345 + frame_index * 8, fill=pale, width=2)
    draw_centered_text(draw, (22, 94, 104, 119), "水源", font(18, True), primary)
    return image


def make_water_source_gif() -> None:
    frames = [
        make_water_source_frame(frame_index).convert(
            "P",
            palette=Image.Palette.ADAPTIVE,
            dither=Image.Dither.NONE,
        )
        for frame_index in range(12)
    ]
    frames[0].save(
        RESOURCE_DIR / "water_source.gif",
        save_all=True,
        append_images=frames[1:],
        duration=90,
        loop=0,
        optimize=False,
        disposal=1,
    )


def flatten_to_rgb(image: Image.Image, background: tuple[int, int, int] | None = None) -> Image.Image:
    if image.mode != "RGBA":
        return image.convert("RGB")
    base = Image.new("RGB", image.size, background or rgb("surface"))
    base.paste(image, mask=image.getchannel("A"))
    return base


def make_overview_panel_frames() -> None:
    frame_dir = RESOURCE_DIR / "overview_anim"
    frame_dir.mkdir(parents=True, exist_ok=True)
    for frame_index in range(12):
        panel = make_overview_panel()
        panel.paste(make_water_source_frame(frame_index).convert("RGBA"), (78, 160))
        panel = panel.crop((0, 0, 1007, 400))
        flatten_to_rgb(panel).save(
            frame_dir / f"overview_{frame_index:02d}.jpg",
            quality=95,
            subsampling=0,
        )


def make_header() -> Image.Image:
    image = Image.new("RGBA", (1048, 144), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rectangle((12, 12, 1035, 95), fill=rgb("header") + (255,))
    draw.rectangle((12, 94, 1035, 95), fill=rgb("border") + (255,))
    draw.rectangle((12, 96, 1035, 97), fill=(232, 236, 241, 210))
    return image


def make_back_icon() -> Image.Image:
    image = Image.new("RGBA", (80, 50), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    color = rgb("primary") + (255,)
    draw.line((50, 11, 29, 25, 50, 39), fill=color, width=3, joint="curve")
    return image


def make_menu_icon(pressed: bool) -> Image.Image:
    image = Image.new("RGBA", (40, 40), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    color = rgb("primary_dark") if pressed else rgb("primary")
    for x in (12, 20, 28):
        draw.ellipse((x - 2, 18, x + 2, 22), fill=color + (255,))
    return image


def make_seekbar(progress: bool) -> Image.Image:
    image = Image.new("RGBA", (300, 24), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    color = rgb("primary") if progress else rgb("control")
    draw.rounded_rectangle((0, 9, 299, 15), 3, fill=color + (255,))
    return image


def make_pump_toggle(enabled: bool) -> Image.Image:
    image = Image.new("RGBA", (90, 45), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    track = rgb("success") if enabled else rgb("control")
    draw.rounded_rectangle((11, 6, 79, 39), 17, fill=track + (255,))
    knob_x = 62 if enabled else 28
    draw.ellipse((knob_x - 14, 8, knob_x + 14, 36), fill=rgb("surface") + (255,))
    return image


def make_gauge() -> Image.Image:
    image = Image.new("RGBA", (65, 65), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.ellipse((4, 4, 60, 60), fill=rgb("surface_soft") + (255,), outline=rgb("border") + (255,), width=2)
    draw.arc((9, 9, 55, 55), 145, 335, fill=rgb("primary") + (255,), width=4)
    draw.line((32, 32, 46, 20), fill=rgb("ink") + (255,), width=3)
    draw.ellipse((28, 28, 36, 36), fill=rgb("ink") + (255,))
    return image


def make_alarm(index: int) -> Image.Image:
    image = Image.new("RGBA", (60, 60), (0, 0, 0, 0))
    icon = render_lucide("clock-3", 44, rgb("primary"))
    image.paste(icon, (8, 4), icon)
    draw = ImageDraw.Draw(image)
    draw.ellipse((42, 39, 58, 55), fill=rgb("primary") + (255,))
    draw_centered_text(draw, (42, 39, 59, 56), str(index), font(11, True), rgb("surface"))
    return image


def make_selection(size: tuple[int, int], selected: bool) -> Image.Image:
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    diameter = min(size) - 6
    left = (size[0] - diameter) // 2
    top = (size[1] - diameter) // 2
    box = (left, top, left + diameter, top + diameter)
    draw.ellipse(
        box,
        fill=(rgb("primary") if selected else rgb("surface")) + (255,),
        outline=(rgb("primary") if selected else rgb("border")) + (255,),
        width=max(2, diameter // 18),
    )
    if selected:
        draw.line(
            (
                left + diameter * 0.25,
                top + diameter * 0.52,
                left + diameter * 0.43,
                top + diameter * 0.69,
                left + diameter * 0.76,
                top + diameter * 0.32,
            ),
            fill=rgb("surface") + (255,),
            width=max(2, diameter // 12),
            joint="curve",
        )
    return image


def make_setting_card(icon_name: str, pressed: bool) -> Image.Image:
    image = Image.new("RGBA", (280, 82), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    shadow = Image.new("RGBA", image.size, (0, 0, 0, 0))
    ImageDraw.Draw(shadow).rounded_rectangle(
        (4, 5, 275, 79),
        8,
        fill=(72, 84, 99, 24),
    )
    image = Image.alpha_composite(image, shadow.filter(ImageFilter.GaussianBlur(3)))
    draw = ImageDraw.Draw(image)
    fill = rgb("primary_soft") if pressed else rgb("surface")
    outline = rgb("primary") if pressed else rgb("border")
    panel = vertical_gradient(
        (278, 80),
        fill,
        rgb("surface_soft") if not pressed else rgb("primary_soft"),
    ).convert("RGBA")
    mask = Image.new("L", panel.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, 277, 79), 8, fill=255)
    image.paste(panel, (1, 1), mask)
    draw.rounded_rectangle((1, 1, 278, 80), 8, outline=outline + (255,), width=1)
    icon = render_lucide(icon_name, 32, rgb("primary"))
    image.paste(icon, (24, 25), icon)
    return image


def make_ime_key(size: tuple[int, int], icon_name: str, pressed: bool) -> Image.Image:
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    fill = rgb("primary_soft") if pressed else rgb("surface")
    outline = rgb("primary") if pressed else rgb("border")
    draw.rounded_rectangle(
        (1, 1, size[0] - 2, size[1] - 2),
        8,
        fill=fill + (255,),
        outline=outline + (255,),
        width=1,
    )
    icon_size = min(30, size[1] - 24)
    icon = render_lucide(icon_name, icon_size, rgb("primary") if pressed else rgb("ink"))
    image.paste(icon, ((size[0] - icon_size) // 2, (size[1] - icon_size) // 2), icon)
    return image


def generate_ime_resources() -> None:
    ime_dir = RESOURCE_DIR / "user_ime"
    ime_dir.mkdir(parents=True, exist_ok=True)
    make_nine_patch(True).resize((26, 26)).save(ime_dir / "key_comm_normal.9.png")
    make_nine_patch(False).resize((26, 26)).save(ime_dir / "key_comm_pressed.9.png")
    for pressed, suffix in ((False, "normal"), (True, "pressed")):
        make_ime_key((64, 64), "delete", pressed).save(ime_dir / f"key_delete_{suffix}.png")
        make_ime_key((64, 64), "languages", pressed).save(ime_dir / f"key_language_switch_{suffix}.png")
        make_ime_key((64, 64), "corner-down-left", pressed).save(ime_dir / f"key_return_{suffix}.png")
        make_ime_key((120, 64), "delete", pressed).save(ime_dir / f"numberkey_delete_{suffix}.png")
        make_ime_key((120, 64), "corner-down-left", pressed).save(ime_dir / f"numberkey_return_{suffix}.png")
    make_selection((50, 50), False).save(ime_dir / "noSelect.png")
    make_selection((50, 50), True).save(ime_dir / "selected.png")


def make_arrow(direction: str) -> Image.Image:
    image = Image.new("RGBA", (259, 259), (0, 0, 0, 0))
    icon = render_lucide(f"chevron-{direction}", 150, rgb("primary"))
    image.paste(icon, (54, 54), icon)
    return image


def tint_backup_resource(name: str, color: tuple[int, int, int]) -> Image.Image:
    source = Image.open(BACKUP_DIR / "resources" / name).convert("RGBA")
    tinted = Image.new("RGBA", source.size, color + (0,))
    tinted.putalpha(source.getchannel("A"))
    return tinted


def generate_resources() -> None:
    background = make_tech_background()
    background.save(RESOURCE_DIR / "background.png")

    middle = make_panel()
    middle.save(RESOURCE_DIR / "bgr_mid.png")
    overview_panel = make_overview_panel()
    overview_panel.paste(make_water_source_frame(0).convert("RGBA"), (78, 160))
    overview_panel = overview_panel.crop((0, 0, 1007, 400))
    flatten_to_rgb(overview_panel).save(
        RESOURCE_DIR / "overview_panel.jpg",
        quality=95,
        subsampling=0,
    )
    make_overview_panel_frames()
    make_water_source_gif()

    make_header().save(RESOURCE_DIR / "nettopwind.png")
    make_back_icon().save(RESOURCE_DIR / "ic_sysbar_back.png")
    make_menu_icon(False).save(RESOURCE_DIR / "menu_more_normal.png")
    make_menu_icon(True).save(RESOURCE_DIR / "menu_more_pressed.png")

    make_nine_patch(True).save(RESOURCE_DIR / "btn_normal.9.png")
    make_nine_patch(False).save(RESOURCE_DIR / "btn_pressed.9.png")

    make_toggle(False).save(RESOURCE_DIR / "toggle-off.png")
    make_toggle(True).save(RESOURCE_DIR / "toggle-on.png")
    make_toggle(False, True).save(RESOURCE_DIR / "toggle-off-invalid.png")

    make_checkbox(False, 32).save(RESOURCE_DIR / "btn_check_off.png")
    make_checkbox(True, 32).save(RESOURCE_DIR / "btn_check_on.png")
    make_checkbox(False, 16).save(RESOURCE_DIR / "check_no.png")
    make_checkbox(True, 16).save(RESOURCE_DIR / "check_yes.png")
    make_radio(False).save(RESOURCE_DIR / "radio_off.png")
    make_radio(True).save(RESOURCE_DIR / "radio_on.png")

    make_seekbar(False).save(RESOURCE_DIR / "jdd.png")
    make_seekbar(True).save(RESOURCE_DIR / "jd.png")
    thumb = Image.new("RGBA", (24, 24), (0, 0, 0, 0))
    thumb_draw = ImageDraw.Draw(thumb)
    thumb_draw.ellipse((2, 2, 21, 21), fill=rgb("primary") + (255,), outline=rgb("surface") + (255,), width=2)
    thumb.save(RESOURCE_DIR / "yb.png")

    make_pump_toggle(False).save(RESOURCE_DIR / "w3_off.png")
    make_pump_toggle(True).save(RESOURCE_DIR / "w3_on.png")
    make_gauge().save(RESOURCE_DIR / "WaterBar.png")
    for index in range(1, 5):
        make_alarm(index).save(RESOURCE_DIR / f"naozhong{index}.png")

    make_selection((40, 40), False).save(RESOURCE_DIR / "f_weixuan.png")
    make_selection((40, 40), True).save(RESOURCE_DIR / "f_xuanzhong.png")
    make_selection((83, 82), False).save(RESOURCE_DIR / "weixuan.png")
    make_selection((83, 82), True).save(RESOURCE_DIR / "yixuan.png")
    make_selection((354, 355), False).save(RESOURCE_DIR / "fuxuankuang.png")
    make_selection((350, 350), True).save(RESOURCE_DIR / "xuanzhong.png")
    make_arrow("left").save(RESOURCE_DIR / "shangyiye.png")
    make_arrow("right").save(RESOURCE_DIR / "xiayiye.png")

    for name in (
        "ic_wifi_signal_0_teal.png",
        "ic_wifi_signal_1_teal.png",
        "ic_wifi_signal_2_teal.png",
        "ic_wifi_signal_3_teal.png",
        "ic_wifi_signal_4_teal.png",
        "ic_wifi_lock_signal_0_teal.png",
        "ic_wifi_lock_signal_1_teal.png",
        "ic_wifi_lock_signal_2_teal.png",
        "ic_wifi_lock_signal_3_teal.png",
        "ic_wifi_lock_signal_4_teal.png",
    ):
        tint_backup_resource(name, rgb("primary")).save(RESOURCE_DIR / name)

    setting_icons = {
        "setting_wifi": "wifi",
        "setting_ethernet": "ethernet-port",
        "setting_cellular": "signal",
        "setting_time": "clock-3",
        "setting_display": "sun",
        "setting_language": "languages",
    }
    for asset, icon_name in setting_icons.items():
        make_setting_card(icon_name, False).save(RESOURCE_DIR / f"{asset}.png")
        make_setting_card(icon_name, True).save(RESOURCE_DIR / f"{asset}_pressed.png")

    for index in range(1, 9):
        make_nav_button(index, False).save(RESOURCE_DIR / f"bgr_btn{index}.png")
        make_nav_button(index, True).save(RESOURCE_DIR / f"bgr_btn{index}_ch.png")
    generate_ime_resources()


def backup_files() -> None:
    ui_backup = BACKUP_DIR / "ui"
    resource_backup = BACKUP_DIR / "resources"
    ui_backup.mkdir(parents=True, exist_ok=True)
    resource_backup.mkdir(parents=True, exist_ok=True)
    for path in UI_DIR.glob("*.ftu"):
        destination = ui_backup / path.name
        if not destination.exists():
            shutil.copy2(path, destination)
    for path in RESOURCE_DIR.rglob("*.png"):
        destination = resource_backup / path.relative_to(RESOURCE_DIR)
        if not destination.exists():
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, destination)


def verify_all() -> None:
    for path in sorted(UI_DIR.glob("*.ftu")):
        data, header, _ = decode_ftu(path)
        encoded = encode_ftu(data, header)
        payload_offset = struct.unpack_from("<I", encoded, 6)[0] + 8
        decoded = json.loads(zlib.decompress(encoded[payload_offset:]).decode("utf-8"))
        if decoded != data:
            raise RuntimeError(f"verification failed for {path}")
        backup_path = BACKUP_DIR / "ui" / path.name
        if backup_path.exists():
            backup_data, _, _ = decode_ftu(backup_path)
            current_signature = [
                item for item in control_signature(data)
                if item != ("WaterSourceImageAnim", 53001)
            ]
            if current_signature != control_signature(backup_data):
                raise RuntimeError(f"control ID or caption changed in {path}")
        print(f"verified {path.name}: {len(encoded)} bytes")


def control_signature(node: object) -> list[tuple[str, int | None]]:
    signature: list[tuple[str, int | None]] = []
    if isinstance(node, dict):
        if "caption" in node:
            signature.append((str(node["caption"]), node.get("id")))
        for value in node.values():
            signature.extend(control_signature(value))
    elif isinstance(node, list):
        for value in node:
            signature.extend(control_signature(value))
    return signature


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify", action="store_true")
    parser.add_argument("--apply", action="store_true")
    parser.add_argument("--restore", action="store_true")
    parser.add_argument("--inspect", action="store_true")
    args = parser.parse_args()

    if args.verify:
        verify_all()
    if args.apply:
        backup_files()
        generate_resources()
        for path in sorted(UI_DIR.glob("*.ftu")):
            apply_style(path)
            print(f"styled {path.name}")
    if args.restore:
        backup_ui = BACKUP_DIR / "ui"
        for path in sorted(backup_ui.glob("*.ftu")):
            shutil.copy2(path, UI_DIR / path.name)
            print(f"restored {path.name}")
        backup_resources = BACKUP_DIR / "resources"
        for path in sorted(backup_resources.rglob("*.png")):
            destination = RESOURCE_DIR / path.relative_to(backup_resources)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, destination)
            print(f"restored {path.relative_to(backup_resources)}")
    if args.inspect:
        for path in sorted(UI_DIR.glob("*.ftu")):
            blob = path.read_bytes()
            payload_offset = struct.unpack_from("<I", blob, 6)[0] + 8
            raw = zlib.decompress(blob[payload_offset:])
            fields = struct.unpack_from("<III", blob, 6)
            print(
                f"{path.name}: size={len(blob)} offset={payload_offset} "
                f"fields={fields} zlib={blob[payload_offset:payload_offset + 2].hex()} "
                f"raw={len(raw)} header={blob[:payload_offset].hex()}"
            )

    if not args.verify and not args.apply and not args.restore and not args.inspect:
        parser.error("choose --verify, --apply, --restore, or --inspect")


if __name__ == "__main__":
    main()
