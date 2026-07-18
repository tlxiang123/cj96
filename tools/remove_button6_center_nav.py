#!/usr/bin/env python3
"""Remove Button6, center the five remaining nav icons, and stabilize pump checkboxes."""

from __future__ import annotations

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "main.ftu"
NAV_CAPTIONS = ("Button1", "Button2", "Button3", "Button4", "Button5")
PUMP_CAPTIONS = tuple(f"PumpCheckbox{index}" for index in range(1, 6))
SCREEN_WIDTH = 1024
NAV_STEP = 120


def find_controls(node: object, captions: set[str]) -> dict[str, dict]:
    found: dict[str, dict] = {}
    if not isinstance(node, dict):
        return found

    caption = node.get("caption")
    if caption in captions:
        found[caption] = node

    for value in node.values():
        if isinstance(value, dict):
            found.update(find_controls(value, captions))
    return found


def remove_control(node: object, caption: str) -> int:
    if not isinstance(node, dict):
        return 0

    removed = 0
    for key, value in list(node.items()):
        if isinstance(value, dict) and value.get("caption") == caption:
            del node[key]
            removed += 1
        elif isinstance(value, dict):
            removed += remove_control(value, caption)
    return removed


def main() -> None:
    data, header, _ = decode_ftu(FTU_PATH)

    if remove_control(data, "Button6") != 1:
        raise RuntimeError("expected exactly one Button6 control")

    controls = find_controls(data, set(NAV_CAPTIONS) | set(PUMP_CAPTIONS))
    missing = (set(NAV_CAPTIONS) | set(PUMP_CAPTIONS)) - controls.keys()
    if missing:
        raise RuntimeError(f"missing controls: {sorted(missing)}")

    group_width = controls[NAV_CAPTIONS[0]]["position"]["width"] + NAV_STEP * 4
    first_left = (SCREEN_WIDTH - group_width) // 2
    for index, caption in enumerate(NAV_CAPTIONS):
        controls[caption]["position"]["left"] = first_left + index * NAV_STEP

    for caption in PUMP_CAPTIONS:
        controls[caption]["picTab"] = {
            "pic0": "window7_select_off.png",
            "pic1": "window7_select_off.png",
            "pic2": "window7_select_on.png",
            "pic3": "window7_select_on.png",
        }

    FTU_PATH.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(FTU_PATH)
    if decoded != data:
        raise RuntimeError("main.ftu round-trip verification failed")

    verified = find_controls(decoded, set(NAV_CAPTIONS) | set(PUMP_CAPTIONS) | {"Button6"})
    if "Button6" in verified:
        raise RuntimeError("Button6 still exists after rewrite")

    positions = [verified[caption]["position"]["left"] for caption in NAV_CAPTIONS]
    print(f"removed Button6; centered {', '.join(NAV_CAPTIONS)} at {positions}")
    print("completed pic0-pic3 states for PumpCheckbox1-PumpCheckbox5")


if __name__ == "__main__":
    main()
