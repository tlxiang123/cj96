#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re
import shutil
import sys
import time

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
UI_DIR = ROOT / "ui"
RESOURCE_DIR = ROOT / "resources"
RELEASE = ROOT / "Release"

WINDOW_ID = 110080
CONTROL_IDS = {
    "W2AddDeviceWindow": WINDOW_ID,
    "W2AddDeviceTitleText": 50280,
    "W2AddDeviceStatusText": 50281,
    "W2AddDeviceAddressLabelText": 50282,
    "W2AddDeviceTypeLabelText": 50283,
    "W2AddDeviceAddressEditText": 51044,
    "W2AddDeviceSensorButton": 20200,
    "W2AddDeviceValveButton": 20201,
    "W2AddDeviceAddressPrevButton": 20202,
    "W2AddDeviceAddressNextButton": 20203,
    "W2AddDeviceCancelButton": 20204,
    "W2AddDeviceOkButton": 20205,
}

BACKGROUND = "w2_add_device_dialog_650x300.png"
TEXT_BLUE = 23483
TEXT_DARK = 0


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    raise LookupError(caption)


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def collect_ids(node: object) -> set[int]:
    return {
        value.get("id")
        for value in walk(node)
        if isinstance(value, dict) and isinstance(value.get("id"), int)
    }


def max_numeric_suffix(node: object, prefix: str) -> int:
    value = 0
    if isinstance(node, dict):
        for key, child in node.items():
            match = re.fullmatch(re.escape(prefix) + r"__(\d+)", key)
            if match:
                value = max(value, int(match.group(1)))
            value = max(value, max_numeric_suffix(child, prefix))
    elif isinstance(node, list):
        for child in node:
            value = max(value, max_numeric_suffix(child, prefix))
    return value


def next_key(root: dict, parent: dict, prefix: str) -> str:
    suffix = max_numeric_suffix(root, prefix) + 1
    while f"{prefix}__{suffix}" in parent:
        suffix += 1
    return f"{prefix}__{suffix}"


def remove_caption(parent: dict, caption: str) -> None:
    for key in list(parent.keys()):
        value = parent[key]
        if isinstance(value, dict) and value.get("caption") == caption:
            del parent[key]


def text_node(caption: str, control_id: int, text: str, left: int, top: int,
              width: int, height: int, font_size: int, color: int = TEXT_BLUE,
              bold: bool = False, alignment: int = 37) -> dict:
    node = {
        "alignment": alignment,
        "caption": caption,
        "colorTab": {"color0": color},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": font_size,
        "id": control_id,
        "text": text,
        "touchable": False,
        "position": {"height": height, "left": left, "top": top, "width": width},
    }
    if bold:
        node["bold"] = True
    return node


def button_node(caption: str, control_id: int, left: int, top: int, width: int,
                height: int, text: str = "", font_size: int = 24,
                pic: str | None = None, selected_pic: str | None = None) -> dict:
    node = {
        "alignment": 37,
        "caption": caption,
        "colorTab": {"color0": TEXT_BLUE, "color1": TEXT_BLUE, "color2": TEXT_BLUE},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": font_size,
        "id": control_id,
        "position": {"height": height, "left": left, "top": top, "width": width},
    }
    if text:
        node["text"] = text
    if pic:
        node["picTab"] = {
            "pic0": pic,
            "pic1": selected_pic or pic,
            "pic2": selected_pic or pic,
        }
    return node


def edit_node(caption: str, control_id: int, text: str, left: int, top: int,
              width: int, height: int) -> dict:
    return {
        "alignment": 37,
        "beepEnable": True,
        "bgColorTab": {"color0": 16181457},
        "caption": caption,
        "colorTab": {"color0": TEXT_BLUE},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 34,
        "hintTextColor": TEXT_BLUE,
        "id": control_id,
        "text": text,
        "textType": 1,
        "position": {"height": height, "left": left, "top": top, "width": width},
    }


def create_background() -> None:
    img = Image.new("RGBA", (650, 300), (247, 252, 255, 255))
    draw = ImageDraw.Draw(img)
    draw.rounded_rectangle(
        (1, 1, 648, 298),
        radius=12,
        fill=(250, 253, 255, 255),
        outline=(0, 132, 255, 255),
        width=3,
    )
    for path in (UI_DIR / BACKGROUND, RESOURCE_DIR / BACKGROUND):
        path.parent.mkdir(parents=True, exist_ok=True)
        img.save(path)


def main() -> int:
    create_background()
    data, header, _ = decode_ftu(MAIN_FTU)
    ids = collect_ids(data)
    for caption, control_id in CONTROL_IDS.items():
        existing = [
            node.get("caption")
            for node in walk(data)
            if isinstance(node, dict) and node.get("id") == control_id
            and node.get("caption") != caption
        ]
        if existing:
            raise RuntimeError(f"id {control_id} collides with {existing[0]}")

    stamp = time.strftime("%Y%m%d_%H%M%S")
    backup_dir = RELEASE / f"w2_add_device_dialog_backup_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup_dir / "main.before.ftu")

    window2 = find_caption(data, "Window2")
    remove_caption(window2, "W2AddDeviceWindow")

    dialog = {
        "backgroundPic": BACKGROUND,
        "beepEnable": True,
        "caption": "W2AddDeviceWindow",
        "id": WINDOW_ID,
        "visible": False,
        "position": {"height": 300, "left": 178, "top": 50, "width": 650},
    }

    dialog[next_key(data, dialog, "textview")] = text_node(
        "W2AddDeviceTitleText", 50280, "添加设备", 0, 14, 650, 36, 26, TEXT_BLUE, True
    )
    dialog[next_key(data, dialog, "textview")] = text_node(
        "W2AddDeviceStatusText", 50281, "", 70, 50, 510, 28, 20, TEXT_BLUE, False
    )
    dialog[next_key(data, dialog, "textview")] = text_node(
        "W2AddDeviceAddressLabelText", 50282, "地址", 250, 84, 70, 36, 24, TEXT_BLUE, True
    )
    dialog[next_key(data, dialog, "button")] = button_node(
        "W2AddDeviceAddressPrevButton", 20202, 185, 76, 60, 58, "<", 34
    )
    dialog[next_key(data, dialog, "edittext")] = edit_node(
        "W2AddDeviceAddressEditText", 51044, "20", 320, 78, 90, 54
    )
    dialog[next_key(data, dialog, "button")] = button_node(
        "W2AddDeviceAddressNextButton", 20203, 425, 76, 60, 58, ">", 34
    )
    dialog[next_key(data, dialog, "textview")] = text_node(
        "W2AddDeviceTypeLabelText", 50283, "解码器类型", 0, 137, 650, 34, 24, TEXT_BLUE, True
    )
    dialog[next_key(data, dialog, "button")] = button_node(
        "W2AddDeviceSensorButton", 20200, 115, 176, 120, 60,
        pic="Sensor.png", selected_pic="Sensor_Selected.png"
    )
    dialog[next_key(data, dialog, "button")] = button_node(
        "W2AddDeviceValveButton", 20201, 415, 176, 120, 60,
        pic="Solenoid_Valve.png", selected_pic="Solenoid_Valve_Selected.png"
    )
    dialog[next_key(data, dialog, "button")] = button_node(
        "W2AddDeviceCancelButton", 20204, 180, 230, 120, 60,
        pic="set_runtime_cancel_same_font_120x60.png"
    )
    dialog[next_key(data, dialog, "button")] = button_node(
        "W2AddDeviceOkButton", 20205, 350, 230, 120, 60,
        pic="set_runtime_ok_same_font_120x60.png"
    )

    window2[next_key(data, window2, "window")] = dialog
    MAIN_FTU.write_bytes(encode_ftu(data, header))
    shutil.copy2(MAIN_FTU, backup_dir / "main.after.ftu")

    verified, _, _ = decode_ftu(MAIN_FTU)
    for caption in CONTROL_IDS:
        find_caption(verified, caption)
    print(f"added W2AddDeviceWindow backup_dir={backup_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
