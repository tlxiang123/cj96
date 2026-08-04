#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys
import time

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, RESOURCE_DIR, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
BACKUP_DIR = ROOT / "Release" / f"before_window5_polished_icons_{time.strftime('%Y%m%d_%H%M%S')}"

BLUE_INT = 23483
WHITE_INT = 0xFFFFFF
BLUE = (0, 128, 255, 238)
BLUE_SOLID = (0, 128, 255, 255)
BLUE_DARK = (0, 93, 180, 255)
WHITE = (255, 255, 255, 255)
SOFT_WHITE = (255, 255, 255, 42)
FONT_FAMILY = "Alibaba-PuHuiTi-Regular"

LEFT_REGION = "window5_region1_frame_430x340.png"
RIGHT_REGION = "window5_region2_frame_405x340.png"

NORMAL_ICONS = {
    "UartValueOnButton": ("Open_Valve.png", "open_valve", "开\n阀", 18),
    "UartValueOffButton": ("Close_Valve.png", "close_valve", "关\n阀", 18),
    "SenserRadioButton": ("Sensor.png", "sensor", "传\n感\n器", 16),
    "ValueRadioButton": ("Solenoid_Valve.png", "solenoid", "电\n磁\n阀", 16),
    "TextView12": ("Source_Address.png", "source_address", "源\n地\n址", 16),
    "TextView17": ("Target_Address.png", "target_address", "目\n标\n地\n址", 15),
    "ChangeAdressOkButton": ("Edit_Address.png", "edit_address", "修\n改\n地\n址", 15),
    "TestAdressOkButton": ("Test_Address.png", "test_address", "测\n试\n地\n址", 15),
    "MustChangeAdressButton": ("Force_Edit.png", "force_edit", "强\n制\n修\n改", 15),
}
SELECTED_ICONS = {
    "SenserRadioButton": "Sensor_Selected.png",
    "ValueRadioButton": "Solenoid_Valve_Selected.png",
}


def find_caption(node: object, caption: str) -> dict | None:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            found = find_caption(value, caption)
            if found is not None:
                return found
    elif isinstance(node, list):
        for value in node:
            found = find_caption(value, caption)
            if found is not None:
                return found
    return None


def draw_scaled(size: tuple[int, int], painter) -> Image.Image:
    scale = 4
    width, height = size
    image = Image.new("RGBA", (width * scale, height * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    def p(value: int | float) -> int:
        return int(round(value * scale))

    def box(values: tuple[int | float, int | float, int | float, int | float]) -> tuple[int, int, int, int]:
        return tuple(p(value) for value in values)

    painter(draw, p, box)
    return image.resize(size, Image.Resampling.LANCZOS)


def make_button_base(size: tuple[int, int], selected: bool = False) -> Image.Image:
    def painter(draw: ImageDraw.ImageDraw, p, box) -> None:
        width, height = size
        rect = box((6, 3, width - 7, height - 4))
        radius = p(11)
        if selected:
            draw.rounded_rectangle(rect, radius=radius, fill=BLUE_SOLID, outline=BLUE_DARK, width=p(2))
            draw.rounded_rectangle(box((10, 7, width - 11, height / 2)), radius=p(8), fill=(255, 255, 255, 32))
        else:
            draw.rounded_rectangle(rect, radius=radius, fill=(0, 0, 0, 0), outline=BLUE, width=p(2))
            draw.rounded_rectangle(box((9, 6, width - 10, height - 7)), radius=p(8), outline=(255, 255, 255, 42), width=p(1))

    return draw_scaled(size, painter)


def draw_icon(kind: str, selected: bool = False, size: tuple[int, int] = (150, 65)) -> Image.Image:
    color = WHITE if selected else BLUE
    dark = WHITE if selected else BLUE_DARK
    image = make_button_base(size, selected)
    scale = 4
    layer = Image.new("RGBA", (size[0] * scale, size[1] * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)

    def p(value: int | float) -> int:
        return int(round(value * scale))

    def xy(points):
        return tuple(p(value) for value in points)

    def line(points, width=3, fill=None):
        draw.line([xy(point) for point in points], fill=fill or color, width=p(width), joint="curve")

    def rounded(rect, radius=4, outline=None, fill=None, width=2):
        draw.rounded_rectangle(xy(rect), radius=p(radius), outline=outline or color, fill=fill, width=p(width))

    def ellipse(rect, outline=None, fill=None, width=2):
        draw.ellipse(xy(rect), outline=outline or color, fill=fill, width=p(width))

    if kind == "open_valve":
        line(((22, 34), (92, 34)), 4)
        rounded((42, 22, 72, 46), 3, width=3)
        line(((57, 22), (57, 12), (76, 12)), 3)
        line(((76, 12), (82, 18), (70, 18), (76, 12)), 2)
        line(((82, 27), (94, 34), (82, 41)), 3)
    elif kind == "close_valve":
        line(((22, 34), (92, 34)), 4)
        rounded((42, 22, 72, 46), 3, width=3)
        line(((57, 22), (57, 12), (76, 12)), 3)
        line(((47, 26), (67, 42)), 3)
        line(((67, 26), (47, 42)), 3)
    elif kind == "sensor":
        line(((28, 41), (50, 25)), 4)
        ellipse((21, 38, 31, 48), fill=color)
        draw.arc(xy((59, 17, 81, 39)), p(-50), p(50), fill=color, width=p(3))
        draw.arc(xy((66, 9, 96, 47)), p(-50), p(50), fill=color, width=p(3))
    elif kind == "solenoid":
        rounded((28, 29, 76, 48), 4, width=3)
        for x in (38, 48, 58, 68):
            line(((x, 29), (x - 8, 48)), 2)
        rounded((65, 20, 83, 36), 3, width=3)
        line(((83, 28), (96, 28)), 3)
        ellipse((93, 25, 99, 31), fill=color)
    elif kind == "source_address":
        rounded((31, 33, 86, 51), 2, width=3)
        line(((37, 39), (51, 27), (66, 39), (80, 27)), 3)
        ellipse((48, 16, 68, 36), width=3)
        ellipse((55, 23, 61, 29), fill=color)
    elif kind == "target_address":
        rounded((30, 38, 86, 52), 2, width=3)
        line(((40, 38), (55, 19), (75, 38)), 3)
        ellipse((74, 27, 91, 44), width=3)
        ellipse((79, 32, 86, 39), fill=color)
    elif kind == "edit_address":
        rounded((30, 20, 76, 48), 3, width=3)
        line(((42, 31), (62, 31)), 2)
        line(((42, 39), (56, 39)), 2)
        line(((62, 45), (88, 19)), 4, dark)
        draw.polygon((xy((86, 17)), xy((94, 25)), xy((89, 29)), xy((81, 21))), fill=dark)
    elif kind == "test_address":
        rounded((30, 19, 84, 49), 7, width=3)
        rounded((41, 26, 73, 42), 3, width=2)
        line(((47, 34), (56, 40), (71, 27)), 3, dark)
    elif kind == "force_edit":
        rounded((28, 31, 82, 49), 4, width=3)
        draw.arc(xy((37, 14, 59, 38)), p(190), p(350), fill=color, width=p(3))
        line(((48, 31), (48, 23)), 3)
        line(((65, 43), (91, 19)), 4, dark)
        draw.polygon((xy((89, 17)), xy((97, 25)), xy((92, 29)), xy((84, 21))), fill=dark)
    else:
        raise ValueError(kind)

    layer = layer.resize(size, Image.Resampling.LANCZOS)
    image.alpha_composite(layer)
    return image


def make_region(path: Path, size: tuple[int, int]) -> None:
    def painter(draw: ImageDraw.ImageDraw, p, box) -> None:
        width, height = size
        rect = box((2, 2, width - 3, height - 3))
        draw.rounded_rectangle(rect, radius=p(10), fill=SOFT_WHITE, outline=(0, 128, 255, 210), width=p(2))
        draw.rounded_rectangle(box((7, 7, width - 8, height - 8)), radius=p(7), outline=(255, 255, 255, 36), width=p(1))

    draw_scaled(size, painter).save(path)


def backup() -> None:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    (BACKUP_DIR / "resources").mkdir(parents=True, exist_ok=True)
    shutil.copy2(UI_DIR / "main.ftu", BACKUP_DIR / "ui" / "main.ftu")
    names = {LEFT_REGION, RIGHT_REGION, *(item[0] for item in NORMAL_ICONS.values()), *SELECTED_ICONS.values()}
    for name in names:
        source = RESOURCE_DIR / name
        if source.is_file():
            shutil.copy2(source, BACKUP_DIR / "resources" / name)


def generate_resources() -> None:
    for _caption, (name, kind, _text, _font_size) in NORMAL_ICONS.items():
        draw_icon(kind, False).save(RESOURCE_DIR / name)
    for caption, name in SELECTED_ICONS.items():
        _normal_name, kind, _text, _font_size = NORMAL_ICONS[caption]
        draw_icon(kind, True).save(RESOURCE_DIR / name)
    make_region(RESOURCE_DIR / LEFT_REGION, (430, 340))
    make_region(RESOURCE_DIR / RIGHT_REGION, (405, 340))


def remove_old_regions(window5: dict) -> list[tuple[str, object]]:
    children: list[tuple[str, object]] = []
    for key, value in list(window5.items()):
        if isinstance(value, dict) and value.get("caption") in {
            "Window5LeftRegion",
            "Window5RightRegion",
            "Window5Region1Window",
            "Window5Region2Window",
        }:
            continue
        children.append((key, value))
    return children


def ensure_regions(window5: dict) -> None:
    metadata_keys = {
        "backgroundColor", "backgroundPic", "beepEnable", "caption", "id",
        "position", "visible", "touchable", "text", "colorTab", "bgColorTab",
    }
    children = remove_old_regions(window5)
    metadata = [(key, value) for key, value in children if key in metadata_keys]
    body = [(key, value) for key, value in children if key not in metadata_keys]
    window5.clear()
    window5.update(metadata)
    window5["window__window5_left_region"] = {
        "caption": "Window5LeftRegion",
        "id": 110063,
        "position": {"left": 30, "top": 27, "width": 430, "height": 340},
        "backgroundPic": LEFT_REGION,
        "touchable": False,
        "beepEnable": False,
    }
    window5["window__window5_right_region"] = {
        "caption": "Window5RightRegion",
        "id": 110064,
        "position": {"left": 575, "top": 27, "width": 405, "height": 340},
        "backgroundPic": RIGHT_REGION,
        "touchable": False,
        "beepEnable": False,
    }
    window5.update(body)


def set_native_label(node: dict, text: str, font_size: int, selected_pic: str | None = None) -> None:
    normal_pic = next(value[0] for caption, value in NORMAL_ICONS.items() if caption == node.get("caption"))
    node["text"] = text
    node["family"] = FONT_FAMILY
    node["fontSize"] = font_size
    node["alignment"] = 37
    node["textPosition"] = {"left": 111, "top": 3, "width": 33, "height": 59}
    node["colorTab"] = {
        "color0": BLUE_INT,
        "color1": BLUE_INT,
        "color2": WHITE_INT if selected_pic else BLUE_INT,
    }
    node["iconPosition"] = {"left": 0, "top": 0, "width": 150, "height": 65}
    node["picTab"] = {"pic0": normal_pic, "pic1": normal_pic, "pic2": selected_pic or normal_pic}
    node.pop("backgroundPic", None)
    node.pop("bgColorTab", None)
    node.pop("textAlign", None)
    node.pop("align", None)
    node.pop("gravity", None)


def set_textview_label(node: dict, text: str, font_size: int, picture: str) -> None:
    node["text"] = text
    node["family"] = FONT_FAMILY
    node["fontSize"] = font_size
    node["alignment"] = 37
    node["textPosition"] = {"left": 111, "top": 3, "width": 33, "height": 59}
    node["colorTab"] = {"color0": BLUE_INT, "color1": BLUE_INT, "color2": BLUE_INT}
    node["backgroundPic"] = picture
    node.pop("picTab", None)
    node.pop("bgColorTab", None)


def set_input(node: dict, left: int, top: int) -> None:
    node["position"] = {"left": left, "top": top, "width": 99, "height": 71}
    node["backgroundPic"] = "debug_edit_inner_99x71.png"
    node["family"] = FONT_FAMILY
    node["fontSize"] = 30
    node["alignment"] = 37
    node["colorTab"] = {"color0": BLUE_INT, "color1": BLUE_INT, "color2": BLUE_INT}
    node.pop("bgColorTab", None)
    node.pop("textPosition", None)


def add_valve_address_input_if_missing(window5: dict) -> dict:
    existing = find_caption(window5, "ValveAddressEditText")
    if existing is not None:
        return existing
    node = {
        "caption": "ValveAddressEditText",
        "id": 51040,
        "text": "20",
        "touchable": True,
    }
    rebuilt: list[tuple[str, object]] = []
    inserted = False
    for key, value in window5.items():
        rebuilt.append((key, value))
        if isinstance(value, dict) and value.get("caption") == "UartValueOffButton":
            rebuilt.append(("edittext__valve_address", node))
            inserted = True
    if not inserted:
        rebuilt.append(("edittext__valve_address", node))
    window5.clear()
    for key, value in rebuilt:
        window5[key] = value
    return node


def apply_ftu() -> None:
    path = UI_DIR / "main.ftu"
    data, header, _ = decode_ftu(path)
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found")

    window5["backgroundPic"] = "bgr_mid.png"
    ensure_regions(window5)

    positions = {
        "UartValueOnButton": {"left": 75, "top": 42, "width": 150, "height": 65},
        "UartValueOffButton": {"left": 265, "top": 42, "width": 150, "height": 65},
        "TextView12": {"left": 597, "top": 58, "width": 150, "height": 65},
        "TextView17": {"left": 835, "top": 58, "width": 150, "height": 65},
        "ChangeAdressOkButton": {"left": 613, "top": 203, "width": 150, "height": 65},
        "TestAdressOkButton": {"left": 791, "top": 203, "width": 150, "height": 65},
        "MustChangeAdressButton": {"left": 693, "top": 282, "width": 150, "height": 65},
    }
    for caption, pos in positions.items():
        node = find_caption(window5, caption)
        if node is None:
            raise RuntimeError(f"{caption} not found")
        node["position"] = pos
        picture, _kind, text, font_size = NORMAL_ICONS[caption]
        if caption.startswith("TextView"):
            set_textview_label(node, text, font_size, picture)
        else:
            set_native_label(node, text, font_size)

    group = find_caption(window5, "RadioGroup1")
    if group is None:
        raise RuntimeError("RadioGroup1 not found")
    group["position"] = {"left": 59, "top": 258, "width": 372, "height": 79}
    radio_positions = {
        "SenserRadioButton": {"left": 0, "top": 10, "width": 150, "height": 65},
        "ValueRadioButton": {"left": 223, "top": 10, "width": 150, "height": 65},
    }
    for caption, pos in radio_positions.items():
        node = find_caption(group, caption)
        if node is None:
            raise RuntimeError(f"{caption} not found")
        node["position"] = pos
        _picture, _kind, text, font_size = NORMAL_ICONS[caption]
        set_native_label(node, text, font_size, SELECTED_ICONS[caption])
        node["checked"] = caption == "ValueRadioButton"

    decoder = find_caption(window5, "Button40")
    if decoder is not None:
        decoder["text"] = "解码器类型：电磁阀"
        decoder["position"] = {"left": 30, "top": 199, "width": 430, "height": 45}
        decoder["family"] = FONT_FAMILY
        decoder["fontSize"] = 24
        decoder["bold"] = True
        decoder["alignment"] = 37
        decoder["colorTab"] = {"color0": BLUE_INT, "color1": BLUE_INT, "color2": BLUE_INT}
        for key in ("picTab", "backgroundPic", "bgColorTab", "iconPosition", "textPosition"):
            decoder.pop(key, None)

    set_input(add_valve_address_input_if_missing(window5), 196, 112)
    for caption, left, top in (
        ("SrouceAddressEditText", 613, 108),
        ("DestAdressEditText", 840, 108),
    ):
        node = find_caption(window5, caption)
        if node is None:
            raise RuntimeError(f"{caption} not found")
        set_input(node, left, top)

    path.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(path)
    if decoded != data:
        raise RuntimeError("main.ftu encode/decode round-trip mismatch")


def verify() -> None:
    data, _, _ = decode_ftu(UI_DIR / "main.ftu")
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 missing")
    for caption in ("Window5LeftRegion", "Window5RightRegion"):
        if find_caption(window5, caption) is None:
            raise RuntimeError(f"{caption} missing")
    for caption, (picture, _kind, text, _font_size) in NORMAL_ICONS.items():
        node = find_caption(window5, caption)
        if node is None:
            raise RuntimeError(f"{caption} missing")
        if node.get("text") != text:
            raise RuntimeError(f"{caption} text was not restored to native label")
        if caption.startswith("TextView"):
            if node.get("backgroundPic") != picture:
                raise RuntimeError(f"{caption} background picture mismatch")
        elif node.get("picTab", {}).get("pic0") != picture:
            raise RuntimeError(f"{caption} picture mismatch")
        if node.get("bgColorTab") is not None:
            raise RuntimeError(f"{caption} still has bgColorTab")
    for caption in ("ValveAddressEditText", "SrouceAddressEditText", "DestAdressEditText"):
        node = find_caption(window5, caption)
        if node.get("position", {}).get("width") != 99 or node.get("position", {}).get("height") != 71:
            raise RuntimeError(f"{caption} input is not 99x71")


def main() -> None:
    backup()
    generate_resources()
    apply_ftu()
    verify()
    print(f"backup={BACKUP_DIR}")
    print("Window5 polished icons, native labels, aligned inputs, and region frames restored")


if __name__ == "__main__":
    main()
