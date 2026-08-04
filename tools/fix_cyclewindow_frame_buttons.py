#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, RESOURCE_DIR, decode_ftu, encode_ftu


MAIN_FTU = UI_DIR / "main.ftu"


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


def make_region(caption: str, control_id: int, left: int, top: int,
                width: int, height: int, pic: str) -> dict:
    return {
        "caption": caption,
        "id": control_id,
        "backgroundPic": pic,
        "touchable": False,
        "position": {
            "left": left,
            "top": top,
            "width": width,
            "height": height,
        },
    }


def set_button_pic(button: dict, pic: str) -> None:
    button["picTab"] = {
        "pic0": pic,
        "pic1": pic,
        "pic2": pic,
    }
    button["iconPosition"] = {
        "left": 0,
        "top": 0,
        "width": button["position"]["width"],
        "height": button["position"]["height"],
    }
    button["text"] = ""


def main() -> None:
    required = [
        "cycle_region_left_390x340.png",
        "cycle_region_right_545x340.png",
        "cycle_ok_confirm_120x60.png",
        "w2_cancel_120x60.png",
    ]
    missing = [name for name in required if not (RESOURCE_DIR / name).is_file()]
    if missing:
        raise FileNotFoundError(", ".join(missing))

    data, header, _ = decode_ftu(MAIN_FTU)
    cycle = find_caption(data, "CycleWindow")
    if cycle is None:
        raise RuntimeError("CycleWindow not found")

    ok_button = find_caption(cycle, "CycleOKButton")
    cancel_button = find_caption(cycle, "CycleCancelButton")
    if ok_button is None or cancel_button is None:
        raise RuntimeError("CycleOKButton or CycleCancelButton not found")

    set_button_pic(ok_button, "cycle_ok_confirm_120x60.png")
    set_button_pic(cancel_button, "w2_cancel_120x60.png")

    scalar_items: list[tuple[str, object]] = []
    child_items: list[tuple[str, object]] = []
    for key, value in cycle.items():
        if isinstance(value, dict):
            if value.get("caption") in {"CycleRegionLeftWindow", "CycleRegionRightWindow"}:
                continue
            child_items.append((key, value))
        else:
            scalar_items.append((key, value))

    left_region = make_region(
        "CycleRegionLeftWindow", 110061, 0, 0, 390, 340,
        "cycle_region_left_390x340.png",
    )
    right_region = make_region(
        "CycleRegionRightWindow", 110062, 445, 0, 545, 340,
        "cycle_region_right_545x340.png",
    )

    cycle.clear()
    for key, value in scalar_items:
        cycle[key] = value
    cycle["window__cycle_region_left"] = left_region
    cycle["window__cycle_region_right"] = right_region
    for key, value in child_items:
        cycle[key] = value

    MAIN_FTU.write_bytes(encode_ftu(data, header))

    decoded, _, _ = decode_ftu(MAIN_FTU)
    decoded_cycle = find_caption(decoded, "CycleWindow")
    if decoded_cycle is None:
        raise RuntimeError("CycleWindow disappeared after encode")
    checks = {
        "left_region": find_caption(decoded_cycle, "CycleRegionLeftWindow"),
        "right_region": find_caption(decoded_cycle, "CycleRegionRightWindow"),
        "ok_button": find_caption(decoded_cycle, "CycleOKButton"),
        "cancel_button": find_caption(decoded_cycle, "CycleCancelButton"),
    }
    if any(value is None for value in checks.values()):
        raise RuntimeError(f"verification failed: {checks}")

    print("updated ui/main.ftu")
    print("CycleRegionLeftWindow -> cycle_region_left_390x340.png")
    print("CycleRegionRightWindow -> cycle_region_right_545x340.png")
    print("CycleOKButton -> cycle_ok_confirm_120x60.png")
    print("CycleCancelButton -> w2_cancel_120x60.png")


if __name__ == "__main__":
    main()
