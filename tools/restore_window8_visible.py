#!/usr/bin/env python3
"""Make Window8 visible in the known-touchable main UI backup."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu
from layout_window3_customer import find_control


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Release" / "device_main_current.ftu"
TARGET = ROOT / "ui" / "main.ftu"


def main() -> None:
    data, header, _ = decode_ftu(SOURCE)
    window8 = find_control(data, "Window8")
    window8["visible"] = True
    TARGET.write_bytes(encode_ftu(data, header))
    print("restored Window8 visibility")


if __name__ == "__main__":
    main()
