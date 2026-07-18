#!/usr/bin/env python3
"""Match Button15 to the 88x88 Window3 program icon."""

from __future__ import annotations

from pathlib import Path

from PIL import Image

from build_window3_customer_icons import extract_blue_art
from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "main.ftu"
SOURCE_ICON = ROOT / "tools" / "assets" / "window3_spray_source.png"
OUTPUT_ICON = ROOT / "resources" / "window3_spray_program_88.png"


def find_control(node: object, caption: str) -> dict | None:
    if not isinstance(node, dict):
        return None
    if node.get("caption") == caption:
        return node
    for value in node.values():
        if isinstance(value, dict):
            found = find_control(value, caption)
            if found is not None:
                return found
    return None


def main() -> None:
    source = Image.open(SOURCE_ICON)
    extract_blue_art(source, (80, 40, 235, 220), 88).save(
        OUTPUT_ICON, optimize=True
    )

    data, header, _ = decode_ftu(FTU_PATH)
    button = find_control(data, "Button15")
    reference = find_control(data, "Window3ProgramIcon")
    cycle_button = find_control(data, "CycleButton")
    if button is None or reference is None or cycle_button is None:
        raise RuntimeError("required Window3 controls are missing")

    button["position"] = dict(reference["position"])
    button["picTab"] = {"pic0": "window3_spray_program_88.png"}
    button["iconPosition"] = {
        "height": 88,
        "left": 0,
        "top": 0,
        "width": 88,
    }
    cycle_button["position"]["left"] = 94

    FTU_PATH.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(FTU_PATH)
    verified = find_control(decoded, "Button15")
    verified_cycle = find_control(decoded, "CycleButton")
    if verified["position"] != reference["position"]:
        raise RuntimeError("Button15 size/position verification failed")
    if verified_cycle["position"]["left"] != 94:
        raise RuntimeError("CycleButton overlap adjustment failed")
    if verified["iconPosition"] != {
        "height": 88, "left": 0, "top": 0, "width": 88
    }:
        raise RuntimeError("Button15 icon must fill its 88x88 canvas")
    print(verified["position"], verified["picTab"], verified["iconPosition"])


if __name__ == "__main__":
    main()
