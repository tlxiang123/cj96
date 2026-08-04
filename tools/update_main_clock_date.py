#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from ftu_style import decode_ftu, encode_ftu


CLOCK_DATE_ID = 50266
CLOCK_COLOR = 0x008FD5


def update_clock_controls(path: Path) -> None:
    data, header, _ = decode_ftu(path)
    clock = data.get("digitalclock__5")
    if not isinstance(clock, dict) or clock.get("caption") != "DigitalClock1":
        raise RuntimeError(f"{path} does not contain top-level DigitalClock1")

    clock["clockColor"] = CLOCK_COLOR
    clock["family"] = "Alibaba-PuHuiTi-Regular"
    clock["fontSize"] = 24
    clock["format"] = "HH:MM:SS"
    clock["touchable"] = False
    clock["position"] = {
        "height": 34,
        "left": 846,
        "top": 16,
        "width": 178,
    }

    data.pop("textview__clock_date", None)
    data["textview__266"] = {
        "alignment": 37,
        "caption": "TextViewClockDate",
        "colorTab": {
            "color0": CLOCK_COLOR,
        },
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 18,
        "id": CLOCK_DATE_ID,
        "text": "2026/7/31",
        "touchable": False,
        "position": {
            "height": 26,
            "left": 846,
            "top": 50,
            "width": 178,
        },
    }

    path.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(path)
    if decoded.get("digitalclock__5", {}).get("position") != clock["position"]:
        raise RuntimeError(f"{path} DigitalClock1 round-trip verification failed")
    if decoded.get("textview__266", {}).get("id") != CLOCK_DATE_ID:
        raise RuntimeError(f"{path} TextViewClockDate round-trip verification failed")
    print(f"updated {path}")


def main() -> None:
    for rel in ("ui/main.ftu", "Release/remote_main.ftu"):
        path = ROOT / rel
        if path.exists():
            update_clock_controls(path)


if __name__ == "__main__":
    main()
