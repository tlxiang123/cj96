#!/usr/bin/env python3
"""Inject one protocol-B touchscreen tap through /dev/input/event0."""

from __future__ import annotations

import argparse
import struct
import subprocess
import time
from pathlib import Path


ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
INPUT_EVENT = "/dev/input/event0"

EV_SYN = 0
EV_KEY = 1
EV_ABS = 3
SYN_REPORT = 0
BTN_TOUCH = 330
ABS_MT_SLOT = 0x2F
ABS_MT_POSITION_X = 0x35
ABS_MT_POSITION_Y = 0x36
ABS_MT_TRACKING_ID = 0x39


def event(event_type: int, code: int, value: int) -> bytes:
    # Target userspace is 32-bit: timeval (2 x int32), type/code, value.
    return struct.pack("<IIHHi", 0, 0, event_type, code, value)


def report(events: list[tuple[int, int, int]]) -> bytes:
    return b"".join(event(*item) for item in events) + event(EV_SYN, SYN_REPORT, 0)


def write_events(payload: bytes) -> None:
    subprocess.run(
        [str(ADB), "-s", SERIAL, "shell", "dd", f"of={INPUT_EVENT}"],
        input=payload,
        check=True,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("x", type=int)
    parser.add_argument("y", type=int)
    args = parser.parse_args()

    subprocess.run(
        [
            str(ADB),
            "-s",
            SERIAL,
            "shell",
            "/tmp/tap_device",
            str(args.x),
            str(args.y),
        ],
        check=True,
    )


if __name__ == "__main__":
    main()
