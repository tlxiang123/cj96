#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys
import time

from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import RESOURCE_DIR, UI_DIR, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
BACKUP_DIR = ROOT / "Release" / f"before_window5_icon_refine_{time.strftime('%Y%m%d_%H%M%S')}"

NORMAL_ICONS = (
    "Open_Valve.png",
    "Close_Valve.png",
    "Sensor.png",
    "Solenoid_Valve.png",
    "Source_Address.png",
    "Target_Address.png",
    "Edit_Address.png",
    "Test_Address.png",
    "Force_Edit.png",
)
SELECTED_ICONS = {
    "Sensor.png": "Sensor_Selected.png",
    "Solenoid_Valve.png": "Solenoid_Valve_Selected.png",
}
LABELS = {
    "Open_Valve.png": "开阀",
    "Close_Valve.png": "关阀",
    "Sensor.png": "传感器",
    "Solenoid_Valve.png": "电磁阀",
    "Source_Address.png": "源地址",
    "Target_Address.png": "目标地址",
    "Edit_Address.png": "修改地址",
    "Test_Address.png": "测试地址",
    "Force_Edit.png": "强制修改",
}

BLUE_INT = 23483
WHITE_INT = 0xFFFFFF
PANEL_BOX = (6, 2, 111, 63)
PANEL_RADIUS = 13
FONT_PATH = ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf"
LEFT_REGION = "window5_region1_frame_430x340.png"
RIGHT_REGION = "window5_region2_frame_405x340.png"


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


def is_blue_pixel(pixel: tuple[int, int, int, int]) -> bool:
    red, green, blue, alpha = pixel
    return alpha > 0 and blue >= 110 and blue > red * 1.28 and blue > green * 1.03


def make_panel_mask() -> Image.Image:
    mask = Image.new("L", (150, 65), 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle(PANEL_BOX, radius=PANEL_RADIUS, fill=255)
    return mask.filter(ImageFilter.GaussianBlur(0.35))


def clean_normal_icon(source: Image.Image) -> Image.Image:
    image = source.convert("RGBA")
    if image.size != (150, 65):
        image = image.resize((150, 65), Image.Resampling.LANCZOS)
    pixels = image.load()
    for y in range(image.height):
        for x in range(image.width):
            red, green, blue, alpha = pixels[x, y]
            # The right-side labels are always redrawn from the real project font.
            if x >= 111:
                pixels[x, y] = (0, 0, 0, 0)
                continue
            score = blue - max(red, green)
            if is_blue_pixel((red, green, blue, alpha)) or score > 35:
                edge_alpha = max(0, min(255, score * 10))
                pixels[x, y] = (20, 112, 201, min(alpha, max(48, edge_alpha)))
            else:
                # No fill at all in normal state: the Window5 background shows through.
                pixels[x, y] = (0, 0, 0, 0)
    return image


def add_vertical_label(image: Image.Image, text: str) -> Image.Image:
    scale = 4
    overlay = Image.new("RGBA", (image.width * scale, image.height * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    count = len(text)
    font_size = 14 if count >= 4 else (16 if count == 3 else 18)
    line_height = 14 if count >= 4 else (18 if count == 3 else 21)
    font = ImageFont.truetype(str(FONT_PATH), font_size * scale)
    total_height = count * line_height * scale
    start_y = (image.height * scale - total_height) // 2 + line_height * scale // 2
    center_x = 130 * scale
    for index, char in enumerate(text):
        draw.text(
            (center_x, start_y + index * line_height * scale),
            char,
            font=font,
            fill=(12, 84, 159, 255),
            stroke_width=2,
            stroke_fill=(12, 84, 159, 255),
            anchor="mm",
        )
    overlay = overlay.resize(image.size, Image.Resampling.LANCZOS)
    return Image.alpha_composite(image, overlay)


def make_selected_icon(normal: Image.Image) -> Image.Image:
    selected = normal.copy().convert("RGBA")
    panel_mask = make_panel_mask()
    mask_pixels = panel_mask.load()
    pixels = selected.load()
    for y in range(selected.height):
        for x in range(selected.width):
            if mask_pixels[x, y] < 128:
                continue
            red, green, blue, alpha = pixels[x, y]
            if alpha == 0:
                continue
            if is_blue_pixel((red, green, blue, alpha)):
                pixels[x, y] = (255, 255, 255, alpha)
            else:
                # A restrained vertical blue fill keeps the selected state clear.
                ratio = y / max(1, selected.height - 1)
                pixels[x, y] = (
                    int(24 + 4 * ratio),
                    int(132 + 23 * ratio),
                    int(218 + 13 * ratio),
                    alpha,
                )

    draw = ImageDraw.Draw(selected)
    draw.rounded_rectangle(PANEL_BOX, radius=PANEL_RADIUS, outline=(10, 104, 187, 255), width=2)
    draw.ellipse((88, 7, 103, 22), fill=(255, 255, 255, 238))
    draw.line((92, 14, 95, 17, 100, 11), fill=(20, 119, 205, 255), width=2, joint="curve")
    return selected


def make_region_frame(path: Path, size: tuple[int, int]) -> None:
    scale = 4
    width, height = size
    image = Image.new("RGBA", (width * scale, height * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    inset = 2 * scale
    draw.rounded_rectangle(
        (inset, inset, width * scale - inset - 1, height * scale - inset - 1),
        radius=10 * scale,
        fill=(0, 0, 0, 0),
        outline=(0, 128, 255, 230),
        width=2 * scale,
    )
    image.resize((width, height), Image.Resampling.LANCZOS).save(path)


def backup() -> None:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    (BACKUP_DIR / "resources").mkdir(parents=True, exist_ok=True)
    shutil.copy2(UI_DIR / "main.ftu", BACKUP_DIR / "ui" / "main.ftu")
    for name in (*NORMAL_ICONS, *SELECTED_ICONS.values(), LEFT_REGION, RIGHT_REGION):
        source = RESOURCE_DIR / name
        if source.is_file():
            shutil.copy2(source, BACKUP_DIR / "resources" / name)


def generate_icons() -> None:
    cleaned: dict[str, Image.Image] = {}
    for name in NORMAL_ICONS:
        path = RESOURCE_DIR / name
        if not path.is_file():
            raise FileNotFoundError(path)
        with Image.open(path) as source:
            normal = clean_normal_icon(source)
        normal = add_vertical_label(normal, LABELS[name])
        normal.save(path)
        cleaned[name] = normal

    for normal_name, selected_name in SELECTED_ICONS.items():
        make_selected_icon(cleaned[normal_name]).save(RESOURCE_DIR / selected_name)
    make_region_frame(RESOURCE_DIR / LEFT_REGION, (430, 340))
    make_region_frame(RESOURCE_DIR / RIGHT_REGION, (405, 340))


def insert_region_frames(window5: dict) -> None:
    for key in list(window5):
        value = window5[key]
        if isinstance(value, dict) and value.get("caption") in {
            "Window5LeftRegion", "Window5RightRegion", "Window5Region1Window", "Window5Region2Window"
        }:
            del window5[key]

    metadata_keys = {
        "backgroundColor", "backgroundPic", "beepEnable", "caption", "id", "position",
        "visible", "touchable", "text", "colorTab", "bgColorTab",
    }
    metadata = [(key, value) for key, value in window5.items() if key in metadata_keys]
    children = [(key, value) for key, value in window5.items() if key not in metadata_keys]
    window5.clear()
    window5.update(metadata)
    window5["window__149"] = {
        "caption": "Window5LeftRegion",
        "id": 110063,
        "position": {"left": 30, "top": 27, "width": 430, "height": 340},
        "backgroundPic": LEFT_REGION,
        "touchable": False,
        "beepEnable": False,
    }
    window5["window__150"] = {
        "caption": "Window5RightRegion",
        "id": 110064,
        "position": {"left": 575, "top": 27, "width": 405, "height": 340},
        "backgroundPic": RIGHT_REGION,
        "touchable": False,
        "beepEnable": False,
    }
    window5.update(children)


def set_image_control(node: dict, normal: str, selected: str | None = None) -> None:
    node["picTab"] = {
        "pic0": normal,
        "pic1": normal,
        "pic2": selected or normal,
    }
    node["iconPosition"] = {"left": 0, "top": 0, "width": 150, "height": 65}
    node.pop("backgroundPic", None)
    node.pop("bgColorTab", None)


def apply_ftu() -> None:
    path = UI_DIR / "main.ftu"
    data, header, _ = decode_ftu(path)
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found")
    insert_region_frames(window5)

    bindings = {
        "UartValueOnButton": "Open_Valve.png",
        "UartValueOffButton": "Close_Valve.png",
        "TextView12": "Source_Address.png",
        "TextView17": "Target_Address.png",
        "ChangeAdressOkButton": "Edit_Address.png",
        "TestAdressOkButton": "Test_Address.png",
        "MustChangeAdressButton": "Force_Edit.png",
    }
    for caption, picture in bindings.items():
        node = find_caption(window5, caption)
        if node is None:
            raise RuntimeError(f"{caption} not found")
        if caption.startswith("TextView"):
            node["backgroundPic"] = picture
            node.pop("picTab", None)
            node.pop("bgColorTab", None)
        else:
            set_image_control(node, picture)

    sensor = find_caption(window5, "SenserRadioButton")
    valve = find_caption(window5, "ValueRadioButton")
    if sensor is None or valve is None:
        raise RuntimeError("Window5 decoder type radio buttons not found")
    set_image_control(sensor, "Sensor.png", "Sensor_Selected.png")
    set_image_control(valve, "Solenoid_Valve.png", "Solenoid_Valve_Selected.png")
    sensor["checked"] = False
    valve["checked"] = True
    sensor["colorTab"] = {"color0": BLUE_INT, "color1": BLUE_INT, "color2": WHITE_INT}
    valve["colorTab"] = {"color0": BLUE_INT, "color1": BLUE_INT, "color2": WHITE_INT}

    path.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(path)
    if decoded != data:
        raise RuntimeError("main.ftu encode/decode round-trip mismatch")


def verify() -> None:
    for name in (*NORMAL_ICONS, *SELECTED_ICONS.values()):
        with Image.open(RESOURCE_DIR / name) as image:
            if image.size != (150, 65) or image.mode != "RGBA":
                raise RuntimeError(f"{name}: expected 150x65 RGBA, got {image.size} {image.mode}")
            if image.getpixel((149, 0))[3] != 0 or image.getpixel((149, 64))[3] != 0:
                raise RuntimeError(f"{name}: outer background is not transparent")

    data, _, _ = decode_ftu(UI_DIR / "main.ftu")
    window5 = find_caption(data, "Window5")
    left = find_caption(window5, "Window5LeftRegion")
    right = find_caption(window5, "Window5RightRegion")
    if left is None or right is None:
        raise RuntimeError("Window5 region frames are missing")
    if left.get("backgroundPic") != LEFT_REGION or right.get("backgroundPic") != RIGHT_REGION:
        raise RuntimeError("Window5 region frame pictures are not bound")
    sensor = find_caption(window5, "SenserRadioButton")
    valve = find_caption(window5, "ValueRadioButton")
    if sensor.get("checked") is not False or valve.get("checked") is not True:
        raise RuntimeError("default decoder selection is not solenoid valve")
    if sensor.get("picTab", {}).get("pic2") != "Sensor_Selected.png":
        raise RuntimeError("sensor selected image is not bound")
    if valve.get("picTab", {}).get("pic2") != "Solenoid_Valve_Selected.png":
        raise RuntimeError("solenoid selected image is not bound")
    for node in (sensor, valve):
        if node.get("iconPosition") != {"left": 0, "top": 0, "width": 150, "height": 65}:
            raise RuntimeError(f"{node.get('caption')} iconPosition mismatch")


def main() -> None:
    backup()
    generate_icons()
    apply_ftu()
    verify()
    print(f"backup={BACKUP_DIR}")
    print("Window5 icons refined; sensor and solenoid selected states bound; solenoid selected by default")


if __name__ == "__main__":
    main()
