#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, RESOURCE_DIR, decode_ftu, encode_ftu

from fix_window5_polished_icons_and_regions import (
    BLUE_INT,
    FONT_FAMILY,
    NORMAL_ICONS,
    SELECTED_ICONS,
    draw_icon,
    find_caption,
)


ROOT = Path(__file__).resolve().parents[1]
BACKUP_DIR = ROOT / "Release" / f"before_cycle_restore_window5_120_{time.strftime('%Y%m%d_%H%M%S')}"

CYCLE_LEFT_FRAME = "cycle_region_left_390x340.png"
CYCLE_RIGHT_FRAME = "cycle_region_right_545x340.png"
CYCLE_COUNT_BOX = "cycle_count_box_208x54.png"
CYCLE_CANCEL_PIC = "w2_group_bind_cancel_120x60.png"
CYCLE_OK_PIC = "w2_ok_120x60.png"


def backup() -> None:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    (BACKUP_DIR / "resources").mkdir(parents=True, exist_ok=True)
    shutil.copy2(UI_DIR / "main.ftu", BACKUP_DIR / "ui" / "main.ftu")
    names = {
        CYCLE_LEFT_FRAME,
        CYCLE_RIGHT_FRAME,
        CYCLE_COUNT_BOX,
        CYCLE_CANCEL_PIC,
        CYCLE_OK_PIC,
        *(item[0] for item in NORMAL_ICONS.values()),
        *SELECTED_ICONS.values(),
    }
    for name in names:
        source = RESOURCE_DIR / name
        if source.is_file():
            shutil.copy2(source, BACKUP_DIR / "resources" / name)


def center_resize_position(node: dict, width: int = 120, height: int = 60) -> None:
    pos = node.get("position")
    if not isinstance(pos, dict):
        return
    old_left = int(pos.get("left", 0))
    old_top = int(pos.get("top", 0))
    old_width = int(pos.get("width", width))
    old_height = int(pos.get("height", height))
    pos["left"] = old_left + (old_width - width) // 2
    pos["top"] = old_top + (old_height - height) // 2
    pos["width"] = width
    pos["height"] = height


def regenerate_window5_120_icons() -> None:
    for _caption, (name, kind, _text, _font_size) in NORMAL_ICONS.items():
        draw_icon(kind, False, (120, 60)).save(RESOURCE_DIR / name)
    for caption, name in SELECTED_ICONS.items():
        _normal_name, kind, _text, _font_size = NORMAL_ICONS[caption]
        draw_icon(kind, True, (120, 60)).save(RESOURCE_DIR / name)


def apply_window5_120(window5: dict) -> None:
    for caption, (normal_pic, _kind, _text, _font_size) in NORMAL_ICONS.items():
        node = find_caption(window5, caption)
        if node is None:
            raise RuntimeError(f"{caption} missing in Window5")
        center_resize_position(node, 120, 60)
        node["iconPosition"] = {"left": 0, "top": 0, "width": 120, "height": 60}
        node["textPosition"] = {"left": 88, "top": 2, "width": 28, "height": 56}
        node["alignment"] = 37
        node["family"] = FONT_FAMILY
        node["colorTab"] = {
            "color0": BLUE_INT,
            "color1": BLUE_INT,
            "color2": 0xFFFFFF if caption in SELECTED_ICONS else BLUE_INT,
        }
        if caption.startswith("TextView"):
            node["backgroundPic"] = normal_pic
            node.pop("picTab", None)
        else:
            selected_pic = SELECTED_ICONS.get(caption)
            node["picTab"] = {
                "pic0": normal_pic,
                "pic1": normal_pic,
                "pic2": selected_pic or normal_pic,
            }
            node.pop("backgroundPic", None)
        node.pop("bgColorTab", None)


def strip_cycle_helpers(cycle: dict) -> list[tuple[str, object]]:
    remove_captions = {
        "CycleLeftRegionFrame",
        "CycleRightRegionFrame",
        "CycleCountEditText",
        "CycleCancelButton",
    }
    remove_ids = {51039, 20110, 50110, 50111}
    body: list[tuple[str, object]] = []
    for key, value in cycle.items():
        if isinstance(value, dict) and (
            value.get("caption") in remove_captions or value.get("id") in remove_ids
        ):
            continue
        body.append((key, value))
    return body


def insert_before_key(items: list[tuple[str, object]], key_prefix: str, new_items: list[tuple[str, object]]) -> list[tuple[str, object]]:
    out: list[tuple[str, object]] = []
    inserted = False
    for key, value in items:
        if not inserted and key.startswith(key_prefix):
            out.extend(new_items)
            inserted = True
        out.append((key, value))
    if not inserted:
        out.extend(new_items)
    return out


def apply_cycle_restore(cycle: dict) -> None:
    metadata_keys = {
        "backgroundColor",
        "backgroundPic",
        "beepEnable",
        "caption",
        "id",
        "position",
        "visible",
        "touchable",
        "text",
        "colorTab",
        "bgColorTab",
    }
    body = strip_cycle_helpers(cycle)
    metadata = [(key, value) for key, value in body if key in metadata_keys]
    controls = [(key, value) for key, value in body if key not in metadata_keys]

    frames = [
        (
            "textview__cycle_left_region_frame",
            {
                "caption": "CycleLeftRegionFrame",
                "id": 50110,
                "position": {"left": 25, "top": 27, "width": 390, "height": 340},
                "backgroundPic": CYCLE_LEFT_FRAME,
                "text": "",
                "touchable": False,
                "beepEnable": False,
            },
        ),
        (
            "textview__cycle_right_region_frame",
            {
                "caption": "CycleRightRegionFrame",
                "id": 50111,
                "position": {"left": 470, "top": 27, "width": 500, "height": 340},
                "backgroundPic": CYCLE_RIGHT_FRAME,
                "text": "",
                "touchable": False,
                "beepEnable": False,
            },
        ),
    ]

    count_control = (
        "edittext__cycle_count",
        {
            "caption": "CycleCountEditText",
            "id": 51039,
            "position": {"left": 145, "top": 292, "width": 145, "height": 60},
            "text": "1",
            "backgroundPic": CYCLE_COUNT_BOX,
            "family": FONT_FAMILY,
            "fontSize": 28,
            "alignment": 37,
            "colorTab": {"color0": BLUE_INT},
            "touchable": True,
        },
    )
    cancel_control = (
        "button__cycle_cancel",
        {
            "caption": "CycleCancelButton",
            "id": 20110,
            "position": {"left": 650, "top": 292, "width": 120, "height": 60},
            "picTab": {"pic0": CYCLE_CANCEL_PIC, "pic1": CYCLE_CANCEL_PIC, "pic2": CYCLE_CANCEL_PIC},
            "touchable": True,
        },
    )

    controls = frames + controls
    controls = insert_before_key(controls, "button__184", [cancel_control])
    # The count box should appear before the OK/cancel buttons but after the time controls.
    controls = insert_before_key(controls, "button__cycle_cancel", [count_control])

    ok = next((value for _key, value in controls if isinstance(value, dict) and value.get("caption") == "CycleOKButton"), None)
    if ok is not None:
        ok["position"] = {"height": 60, "left": 800, "top": 292, "width": 120}
        ok["picTab"] = {"pic0": CYCLE_OK_PIC, "pic1": CYCLE_OK_PIC, "pic2": CYCLE_OK_PIC}
        ok.pop("backgroundPic", None)
        ok.pop("bgColorTab", None)

    cycle.clear()
    cycle.update(metadata)
    cycle.update(controls)


def verify(data: dict) -> None:
    cycle = find_caption(data, "CycleWindow")
    window5 = find_caption(data, "Window5")
    if cycle is None or window5 is None:
        raise RuntimeError("CycleWindow or Window5 missing")
    for caption, expected_id in {
        "CycleLeftRegionFrame": 50110,
        "CycleRightRegionFrame": 50111,
        "CycleCountEditText": 51039,
        "CycleCancelButton": 20110,
        "CycleOKButton": 20076,
    }.items():
        node = find_caption(cycle, caption)
        if node is None:
            raise RuntimeError(f"{caption} missing")
        if node.get("id") != expected_id:
            raise RuntimeError(f"{caption} id mismatch")
    for caption in NORMAL_ICONS:
        node = find_caption(window5, caption)
        pos = node.get("position", {})
        if pos.get("width") != 120 or pos.get("height") != 60:
            raise RuntimeError(f"{caption} is not 120x60: {pos}")


def main() -> None:
    backup()
    regenerate_window5_120_icons()

    path = UI_DIR / "main.ftu"
    data, header, _ = decode_ftu(path)
    cycle = find_caption(data, "CycleWindow")
    window5 = find_caption(data, "Window5")
    if cycle is None or window5 is None:
        raise RuntimeError("CycleWindow or Window5 missing")
    apply_cycle_restore(cycle)
    apply_window5_120(window5)
    path.write_bytes(encode_ftu(data, header))

    decoded, _, _ = decode_ftu(path)
    verify(decoded)
    print(f"restored CycleWindow controls and resized Window5 icons to 120x60; backup={BACKUP_DIR}")


if __name__ == "__main__":
    main()
