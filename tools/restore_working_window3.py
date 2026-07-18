#!/usr/bin/env python3
"""Restore only Window3 from the last device-verified main.ftu backup."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu
from layout_window3_customer import find_control


ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "ui" / "main.ftu"
BACKUP = ROOT / "Release" / "device_main_current.ftu"


def replace_window3(node: object, replacement: dict) -> bool:
    if isinstance(node, dict):
        for key, value in node.items():
            if isinstance(value, dict) and value.get("caption") == "Window3":
                node[key] = replacement
                return True
            if replace_window3(value, replacement):
                return True
    elif isinstance(node, list):
        for value in node:
            if replace_window3(value, replacement):
                return True
    return False


def main() -> None:
    current, header, _ = decode_ftu(TARGET)
    working, _, _ = decode_ftu(BACKUP)
    window3 = find_control(working, "Window3")
    if not replace_window3(current, window3):
        raise RuntimeError("Window3 was not found in main.ftu")
    TARGET.write_bytes(encode_ftu(current, header))
    print("restored device-verified Window3")


if __name__ == "__main__":
    main()
