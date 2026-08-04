#!/usr/bin/env python3
"""Pull device screenshots to the Windows desktop tmppic folder.

The UI button saves /mnt/extsd/cj96_screenshot.bmp on the target. Keep this
script running on the PC; it watches that file via ADB and copies each new
version to the user's Desktop/tmppic folder as a PNG when Pillow is available.
"""

from __future__ import annotations

import os
import subprocess
import sys
import time
import argparse
from datetime import datetime
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = os.environ.get("ADB_SERIAL", "192.168.1.70:5555")
REMOTE_SCREENSHOT = "/mnt/extsd/cj96_screenshot.bmp"
SCREENSHOT_DIR = Path.home() / "Desktop" / "tmppic"


def adb(*args: str, check: bool = True) -> subprocess.CompletedProcess[bytes]:
    return subprocess.run(
        [str(ADB), "-s", SERIAL, *args],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=check,
    )


def remote_signature() -> str | None:
    result = adb(
        "shell",
        "ls",
        "-l",
        REMOTE_SCREENSHOT,
        check=False,
    )
    if result.returncode != 0:
        return None
    text = result.stdout.decode("utf-8", errors="replace").strip()
    if not text or "No such file" in text:
        return None
    return text


def pull_once() -> Path:
    SCREENSHOT_DIR.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    bmp_path = SCREENSHOT_DIR / f"cj96_screenshot_{stamp}.bmp"
    adb("pull", REMOTE_SCREENSHOT, str(bmp_path))

    try:
        from PIL import Image
    except Exception:
        return bmp_path

    png_path = bmp_path.with_suffix(".png")
    Image.open(bmp_path).save(png_path)
    try:
        bmp_path.unlink()
    except OSError:
        pass
    return png_path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--once", action="store_true", help="pull the current screenshot and exit")
    args = parser.parse_args()

    print(f"Watching {SERIAL}:{REMOTE_SCREENSHOT}")
    print(f"Screenshot output: {SCREENSHOT_DIR}")
    last_signature = None

    if args.once:
        if not remote_signature():
            print("No screenshot file found on target.")
            return 1
        output = pull_once()
        print(f"Saved {output}")
        return 0

    while True:
        signature = remote_signature()
        if signature and signature != last_signature:
            last_signature = signature
            output = pull_once()
            print(f"Saved {output}")
        time.sleep(1)


if __name__ == "__main__":
    raise SystemExit(main())
