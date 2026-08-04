#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, RESOURCE_DIR, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
BACKUP_DIR = ROOT / "Release" / f"before_window5_regions_valve_addr_{time.strftime('%Y%m%d_%H%M%S')}"

BLUE = 23483
BLACK = 0

LEFT_REGION_PIC = "debug_region_left_430x340.png"
RIGHT_REGION_PIC = "debug_region_right_405x340.png"
OUTLINE_150 = "debug_outline_150x65.png"
OUTLINE_148 = "debug_outline_148x71.png"
OUTLINE_99 = "debug_outline_99x71.png"
OUTLINE_368 = "debug_outline_368x67.png"
OUTLINE_149 = "debug_outline_149x59.png"


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


def walk(node: object) -> list[dict]:
    out: list[dict] = []
    if isinstance(node, dict):
        if "caption" in node:
            out.append(node)
        for value in node.values():
            out.extend(walk(value))
    elif isinstance(node, list):
        for value in node:
            out.extend(walk(value))
    return out


def set_pic_button(node: dict, pic: str, color: int = BLUE) -> None:
    node.pop("bgColorTab", None)
    node["picTab"] = {"pic0": pic, "pic1": pic, "pic2": pic}
    node["colorTab"] = {"color0": color, "color1": color, "color2": color}
    pos = node.get("position", {})
    node["iconPosition"] = {
        "left": 0,
        "top": 0,
        "width": int(pos.get("width", 0)),
        "height": int(pos.get("height", 0)),
    }


def set_bg_control(node: dict, pic: str, text_color: int = BLACK) -> None:
    node.pop("bgColorTab", None)
    node["backgroundPic"] = pic
    node["colorTab"] = {"color0": text_color, "color1": text_color, "color2": text_color}


def make_region(caption: str, control_id: int, left: int, top: int,
                width: int, height: int, pic: str) -> dict:
    return {
        "caption": caption,
        "id": control_id,
        "backgroundPic": pic,
        "touchable": False,
        "position": {"left": left, "top": top, "width": width, "height": height},
    }


def make_valve_address_edit() -> dict:
    return {
        "caption": "ValveAddressEditText",
        "id": 51040,
        "text": "20",
        "backgroundPic": OUTLINE_99,
        "colorTab": {"color0": BLACK},
        "fontSize": 32,
        "position": {"left": 178, "top": 120, "width": 99, "height": 71},
    }


def add_or_replace_regions(window5: dict) -> None:
    scalar_items: list[tuple[str, object]] = []
    child_items: list[tuple[str, object]] = []
    for key, value in list(window5.items()):
        if isinstance(value, dict):
            if value.get("caption") in {"Window5LeftRegion", "Window5RightRegion"}:
                continue
            child_items.append((key, value))
        else:
            scalar_items.append((key, value))

    window5.clear()
    for key, value in scalar_items:
        window5[key] = value
    window5["window__window5_left_region"] = make_region(
        "Window5LeftRegion", 110063, 30, 27, 430, 340, LEFT_REGION_PIC,
    )
    window5["window__window5_right_region"] = make_region(
        "Window5RightRegion", 110064, 575, 38, 405, 340, RIGHT_REGION_PIC,
    )
    for key, value in child_items:
        window5[key] = value


def add_or_replace_valve_address(window5: dict) -> None:
    existing_key = None
    for key, value in window5.items():
        if isinstance(value, dict) and value.get("caption") == "ValveAddressEditText":
            existing_key = key
            break
    node = make_valve_address_edit()
    if existing_key is not None:
        window5[existing_key] = {**window5[existing_key], **node}
    else:
        # Add after valve buttons by rebuilding insertion order.
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


def apply_window5_style(data: dict) -> None:
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found")

    add_or_replace_regions(window5)
    add_or_replace_valve_address(window5)

    for caption, pic in {
        "UartValueOnButton": OUTLINE_150,
        "UartValueOffButton": OUTLINE_150,
        "TestAdressOkButton": OUTLINE_148,
        "ChangeAdressOkButton": OUTLINE_148,
        "MustChangeAdressButton": OUTLINE_148,
        "Button40": OUTLINE_368,
    }.items():
        node = find_caption(window5, caption)
        if node is not None:
            set_pic_button(node, pic, BLUE)

    for caption in {"SrouceAddressEditText", "DestAdressEditText", "ValveAddressEditText"}:
        node = find_caption(window5, caption)
        if node is not None:
            set_bg_control(node, OUTLINE_99, BLACK)

    for caption in {"SenserRadioButton", "ValueRadioButton"}:
        node = find_caption(window5, caption)
        if node is not None:
            set_pic_button(node, OUTLINE_149, BLUE)


def backup() -> None:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    (BACKUP_DIR / "resources").mkdir(parents=True, exist_ok=True)
    shutil.copy2(UI_DIR / "main.ftu", BACKUP_DIR / "ui" / "main.ftu")
    for name in {
        LEFT_REGION_PIC, RIGHT_REGION_PIC, OUTLINE_150, OUTLINE_148,
        OUTLINE_99, OUTLINE_368, OUTLINE_149,
    }:
        source = RESOURCE_DIR / name
        if source.is_file():
            shutil.copy2(source, BACKUP_DIR / "resources" / name)


def verify(data: dict) -> None:
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found after apply")
    required = {
        "Window5LeftRegion",
        "Window5RightRegion",
        "UartValueOnButton",
        "UartValueOffButton",
        "ValveAddressEditText",
        "Button40",
        "SenserRadioButton",
        "ValueRadioButton",
        "SrouceAddressEditText",
        "DestAdressEditText",
    }
    captions = {str(node.get("caption")) for node in walk(window5)}
    missing = sorted(required - captions)
    if missing:
        raise RuntimeError(f"missing controls after apply: {missing}")
    for caption in {
        "Button40", "SenserRadioButton", "ValueRadioButton",
        "SrouceAddressEditText", "DestAdressEditText", "ValveAddressEditText",
    }:
        node = find_caption(window5, caption)
        if node and "bgColorTab" in node:
            raise RuntimeError(f"{caption} still has bgColorTab")


def main() -> None:
    required = {
        LEFT_REGION_PIC, RIGHT_REGION_PIC, OUTLINE_150, OUTLINE_148,
        OUTLINE_99, OUTLINE_368, OUTLINE_149,
    }
    missing = sorted(name for name in required if not (RESOURCE_DIR / name).is_file())
    if missing:
        raise FileNotFoundError(", ".join(missing))
    backup()
    path = UI_DIR / "main.ftu"
    data, header, _ = decode_ftu(path)
    apply_window5_style(data)
    verify(data)
    path.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(path)
    verify(decoded)
    if decoded != data:
        raise RuntimeError("round-trip mismatch")
    print(f"backup={BACKUP_DIR}")
    print("updated Window5 regions, valve address edit, and removed tan bgColorTab from Window5 controls")


if __name__ == "__main__":
    main()
