#!/usr/bin/env python3
"""Capture both 1024x600 BGRA framebuffer pages from the target."""

from __future__ import annotations

import subprocess
import os
import shutil
from pathlib import Path

from PIL import Image


ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = os.environ.get("ADB_SERIAL", "192.168.1.103:5555")
WIDTH = 1024
HEIGHT = 600
PAGE_BYTES = WIDTH * HEIGHT * 4
OUTPUT_DIR = Path(__file__).resolve().parents[1] / "Release"


def adb(*args: str, stdout: int | None = subprocess.PIPE) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        [str(ADB), "-s", SERIAL, *args],
        check=True,
        stdout=stdout,
    )


def ui_pid() -> str:
    output = adb("shell", "ps").stdout.decode("utf-8", errors="replace")
    for line in output.splitlines():
        if line.rstrip().endswith("/bin/zkgui"):
            return line.split()[1]
    raise RuntimeError("zkgui process not found")


def main() -> None:
    stop_ui = os.environ.get("CAPTURE_STOP_UI", "1") != "0"
    if stop_ui:
        pid = ui_pid()
        adb("shell", "kill", "-STOP", pid)
        try:
            result = adb("shell", "/tmp/fb_capture")
        finally:
            adb("shell", "kill", "-CONT", pid)
    else:
        result = adb("shell", "/tmp/fb_capture")
    if len(result.stdout) < PAGE_BYTES:
        raise RuntimeError(f"short framebuffer read: {len(result.stdout)} bytes")

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    page_count = max(1, len(result.stdout) // PAGE_BYTES)
    outputs: list[Path] = []
    for index in range(min(page_count, 2)):
        offset = index * PAGE_BYTES
        image = Image.frombytes(
            "RGBA",
            (WIDTH, HEIGHT),
            result.stdout[offset:offset + PAGE_BYTES],
            "raw",
            "BGRA",
        )
        image.putalpha(255)
        output = OUTPUT_DIR / f"framebuffer_page{index}.png"
        image.save(output)
        outputs.append(output)

    active = OUTPUT_DIR / "framebuffer_active.png"
    shutil.copyfile(outputs[0], active)
    for output in outputs:
        print(output)
    print(active)


if __name__ == "__main__":
    main()
