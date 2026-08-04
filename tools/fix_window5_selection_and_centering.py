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
BACKUP_DIR = ROOT / "Release" / f"before_window5_selection_center_{time.strftime('%Y%m%d_%H%M%S')}"

BLUE_INT = 23483
WHITE_INT = 0xFFFFFF
BLUE = (0, 128, 255, 255)
WHITE = (255, 255, 255, 255)
RADIO_NORMAL = "debug_radio_149x59.png"
RADIO_SELECTED = "debug_radio_149x59_selected.png"


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


def draw_radio(path: Path, selected: bool) -> None:
    scale = 4
    width, height = 149, 59
    image = Image.new("RGBA", (width * scale, height * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    outline_width = 2 * scale
    box = (
        outline_width // 2,
        outline_width // 2,
        width * scale - outline_width // 2 - 1,
        height * scale - outline_width // 2 - 1,
    )
    draw.rounded_rectangle(
        box,
        radius=8 * scale,
        fill=BLUE if selected else WHITE,
        outline=BLUE,
        width=outline_width,
    )
    image = image.resize((width, height), Image.Resampling.LANCZOS)
    image.save(path)


def set_pic_button(node: dict, pic0: str, pic2: str | None = None) -> None:
    selected = pic2 or pic0
    node["picTab"] = {"pic0": pic0, "pic1": pic0, "pic2": selected}
    node["colorTab"] = {
        "color0": BLUE_INT,
        "color1": BLUE_INT,
        "color2": WHITE_INT if selected != pic0 else BLUE_INT,
    }
    node["alignment"] = 37
    node.pop("bgColorTab", None)
    pos = node.get("position") or {}
    node["iconPosition"] = {
        "left": 0,
        "top": 0,
        "width": int(pos.get("width", 0)),
        "height": int(pos.get("height", 0)),
    }


def set_center_text(node: dict, blue: bool = True) -> None:
    node["alignment"] = 37
    if blue:
        node["colorTab"] = {"color0": BLUE_INT, "color1": BLUE_INT, "color2": BLUE_INT}


def backup() -> None:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    (BACKUP_DIR / "resources").mkdir(parents=True, exist_ok=True)
    shutil.copy2(UI_DIR / "main.ftu", BACKUP_DIR / "ui" / "main.ftu")
    for name in (RADIO_NORMAL, RADIO_SELECTED, "debug_outline_149x59.png"):
        source = RESOURCE_DIR / name
        if source.is_file():
            shutil.copy2(source, BACKUP_DIR / "resources" / name)


def apply_ftu() -> None:
    path = UI_DIR / "main.ftu"
    data, header, _ = decode_ftu(path)
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found")

    on = find_caption(window5, "UartValueOnButton")
    off = find_caption(window5, "UartValueOffButton")
    if on is not None:
        on["position"] = {"left": 75, "top": 42, "width": 150, "height": 65}
        set_pic_button(on, "debug_outline_150x65.png")
    if off is not None:
        off["position"] = {"left": 265, "top": 42, "width": 150, "height": 65}
        set_pic_button(off, "debug_outline_150x65.png")

    valve_edit = find_caption(window5, "ValveAddressEditText")
    if valve_edit is not None:
        valve_edit["position"] = {"left": 196, "top": 112, "width": 99, "height": 71}
        set_center_text(valve_edit)
        valve_edit["fontSize"] = 30

    decoder = find_caption(window5, "Button40")
    if decoder is not None:
        decoder["text"] = "解码器类型：电磁阀"
        decoder["position"] = {"left": 30, "top": 199, "width": 430, "height": 45}
        decoder.pop("picTab", None)
        decoder.pop("backgroundPic", None)
        decoder.pop("bgColorTab", None)
        decoder.pop("iconPosition", None)
        decoder["fontSize"] = 24
        decoder["bold"] = True
        set_center_text(decoder)

    group = find_caption(window5, "RadioGroup1")
    if group is not None:
        group["position"] = {"left": 59, "top": 258, "width": 372, "height": 79}
        sensor = find_caption(group, "SenserRadioButton")
        valve = find_caption(group, "ValueRadioButton")
        if sensor is not None:
            sensor["position"] = {"left": 0, "top": 10, "width": 149, "height": 59}
            set_pic_button(sensor, RADIO_NORMAL, RADIO_SELECTED)
        if valve is not None:
            valve["position"] = {"left": 223, "top": 10, "width": 149, "height": 59}
            set_pic_button(valve, RADIO_NORMAL, RADIO_SELECTED)

    for caption in (
        "SrouceAddressEditText", "DestAdressEditText",
        "TextView12", "TextView17",
        "ChangeAdressOkButton", "TestAdressOkButton", "MustChangeAdressButton",
    ):
        node = find_caption(window5, caption)
        if node is not None:
            set_center_text(node)
            if caption.endswith("Button"):
                pos = node.get("position") or {}
                node["iconPosition"] = {
                    "left": 0,
                    "top": 0,
                    "width": int(pos.get("width", 0)),
                    "height": int(pos.get("height", 0)),
                }

    path.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(path)
    if decoded != data:
        raise RuntimeError("round-trip mismatch")


def verify() -> None:
    data, _, _ = decode_ftu(UI_DIR / "main.ftu")
    window5 = find_caption(data, "Window5")
    checks = {
        "UartValueOnButton": {"left": 75},
        "UartValueOffButton": {"left": 265},
        "ValveAddressEditText": {"left": 196},
        "RadioGroup1": {"left": 59},
    }
    for caption, expected in checks.items():
        node = find_caption(window5, caption)
        if node is None:
            raise RuntimeError(f"{caption} missing")
        for key, value in expected.items():
            if int(node.get("position", {}).get(key, -1)) != value:
                raise RuntimeError(f"{caption} position.{key} mismatch")
    decoder = find_caption(window5, "Button40")
    if decoder.get("text") != "解码器类型：电磁阀":
        raise RuntimeError("decoder title text mismatch")
    for caption in ("SenserRadioButton", "ValueRadioButton"):
        node = find_caption(window5, caption)
        if node.get("picTab", {}).get("pic2") != RADIO_SELECTED:
            raise RuntimeError(f"{caption} missing selected picture")
        if node.get("colorTab", {}).get("color2") != WHITE_INT:
            raise RuntimeError(f"{caption} selected text is not white")
        if node.get("alignment") != 37:
            raise RuntimeError(f"{caption} not centered")


def main() -> None:
    backup()
    draw_radio(RESOURCE_DIR / RADIO_NORMAL, False)
    draw_radio(RESOURCE_DIR / RADIO_SELECTED, True)
    apply_ftu()
    verify()
    print(f"backup={BACKUP_DIR}")
    print("Window5 selection state and centering updated")


if __name__ == "__main__":
    main()
