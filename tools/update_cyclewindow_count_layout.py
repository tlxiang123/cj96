#!/usr/bin/env python3
"""Lay out the board-side CycleWindow cycle count control.

The existing CycleCountEditText ID and callbacks are retained so the current
page6 calculation logic receives user input without regenerated bindings.
"""

from __future__ import annotations

import copy
import shutil
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ftu_style import UI_DIR, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = UI_DIR / "main.ftu"
BACKUP_DIR = ROOT / "Release" / f"before_cycle_count_layout_{time.strftime('%Y%m%d_%H%M%S')}"

BLUE = 23483
FONT_FAMILY = "Alibaba-PuHuiTi-Regular"

SECOND_RANGE_CAPTIONS = {
    "Cycle2MinEndEditText",
    "Button29",
    "Cycle2HourStartEditText",
    "Cycle2MinStartEditText",
    "Button30",
    "Cycle2HourEndEditText",
}
COUNT_HELPER_CAPTIONS = {"CycleCountLabel", "CycleCountUnitText"}


def find_caption(node: object, caption: str) -> dict | None:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for child in node.values():
            found = find_caption(child, caption)
            if found is not None:
                return found
    elif isinstance(node, list):
        for child in node:
            found = find_caption(child, caption)
            if found is not None:
                return found
    return None


def remove_cycle_children(cycle: dict, captions: set[str]) -> None:
    for key, value in list(cycle.items()):
        if isinstance(value, dict) and value.get("caption") in captions:
            del cycle[key]


def move_second_range_up(cycle: dict) -> None:
    for caption in SECOND_RANGE_CAPTIONS:
        control = find_caption(cycle, caption)
        if control is None:
            raise RuntimeError(f"{caption} is missing")
        position = control.get("position")
        if not isinstance(position, dict):
            raise RuntimeError(f"{caption} has no position")
        position["top"] = 214 if caption == "Button29" else 224


def build_count_label() -> dict:
    return {
        "caption": "CycleCountLabel",
        "id": 50112,
        "position": {"left": 82, "top": 302, "width": 105, "height": 40},
        "text": "喷雾次数",
        "family": FONT_FAMILY,
        "fontSize": 21,
        "alignment": 37,
        "colorTab": {"color0": BLUE, "color1": BLUE},
        "touchable": False,
        "beepEnable": False,
    }


def build_count_edit(interval_edit: dict) -> dict:
    # Preserve the interval field's IME configuration. It is the board's
    # existing numeric-input edit control, so the count field opens the same
    # numeric keypad by default.
    count_edit = copy.deepcopy(interval_edit)
    count_edit.update(
        {
            "caption": "CycleCountEditText",
            "id": 51039,
            "position": {"left": 194, "top": 302, "width": 80, "height": 40},
            "text": "3",
            "backgroundPic": "cycle_edit_80x40.png",
            "family": FONT_FAMILY,
            "fontSize": 28,
            "alignment": 37,
            "colorTab": {"color0": BLUE},
            "touchable": True,
        }
    )
    return count_edit


def build_count_unit() -> dict:
    return {
        "caption": "CycleCountUnitText",
        "id": 50113,
        "position": {"left": 281, "top": 303, "width": 36, "height": 38},
        "text": "次",
        "family": FONT_FAMILY,
        "fontSize": 18,
        "alignment": 37,
        "colorTab": {"color0": BLUE, "color1": BLUE},
        "touchable": False,
        "beepEnable": False,
    }


def verify(data: dict) -> None:
    cycle = find_caption(data, "CycleWindow")
    if cycle is None:
        raise RuntimeError("CycleWindow is missing")

    expected_positions = {
        "Button29": {"left": 38, "top": 214, "width": 60, "height": 60},
        "Cycle2HourStartEditText": {"left": 113, "top": 224, "width": 46, "height": 40},
        "CycleCountEditText": {"left": 194, "top": 302, "width": 80, "height": 40},
    }
    for caption, expected_position in expected_positions.items():
        control = find_caption(cycle, caption)
        if control is None:
            raise RuntimeError(f"{caption} is missing after update")
        if control.get("position") != expected_position:
            raise RuntimeError(f"{caption} position mismatch: {control.get('position')}")

    count = find_caption(cycle, "CycleCountEditText")
    if count.get("id") != 51039 or count.get("backgroundPic") != "cycle_edit_80x40.png":
        raise RuntimeError("CycleCountEditText binding or style mismatch")
    if count.get("text") != "3":
        raise RuntimeError("CycleCountEditText default is not 3")


def main() -> None:
    if not MAIN_FTU.is_file():
        raise FileNotFoundError(MAIN_FTU)

    BACKUP_DIR.mkdir(parents=True, exist_ok=True)
    shutil.copy2(MAIN_FTU, BACKUP_DIR / "main.ftu")

    data, header, _ = decode_ftu(MAIN_FTU)
    cycle = find_caption(data, "CycleWindow")
    if cycle is None:
        raise RuntimeError("CycleWindow is missing")

    remove_cycle_children(cycle, COUNT_HELPER_CAPTIONS | {"CycleCountEditText"})
    move_second_range_up(cycle)

    interval_edit = find_caption(cycle, "IntervalEditText")
    if interval_edit is None:
        raise RuntimeError("IntervalEditText is missing")

    numeric_keys = {"textview__900", "edittext__901", "textview__902"}
    collisions = numeric_keys.intersection(cycle)
    if collisions:
        raise RuntimeError(f"FTU control-key collision: {sorted(collisions)}")

    # Use FlyThingsIDE's native numeric control-key shape so the two labels
    # remain editable in the IDE control tree.
    cycle["textview__900"] = build_count_label()
    cycle["edittext__901"] = build_count_edit(interval_edit)
    cycle["textview__902"] = build_count_unit()

    MAIN_FTU.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(MAIN_FTU)
    verify(decoded)
    print(f"updated CycleWindow; backup={BACKUP_DIR}")


if __name__ == "__main__":
    main()
