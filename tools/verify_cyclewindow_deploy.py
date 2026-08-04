#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
REMOTE_UI = "/mnt/extsd/ui"


FILES = [
    ("ui/main.ftu", "main.ftu"),
    ("resources/cycle_region_left_390x340.png", "cycle_region_left_390x340.png"),
    ("resources/cycle_region_right_545x340.png", "cycle_region_right_545x340.png"),
    ("resources/cycle_ok_confirm_120x60.png", "cycle_ok_confirm_120x60.png"),
    ("resources/w2_cancel_120x60.png", "w2_cancel_120x60.png"),
]


def md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def main() -> None:
    temp_dir = Path(tempfile.gettempdir()) / "cj96-cyclewindow-verify"
    temp_dir.mkdir(parents=True, exist_ok=True)
    for local_name, remote_name in FILES:
        remote_copy = temp_dir / remote_name
        subprocess.run([
            str(ADB), "-s", SERIAL, "pull",
            f"{REMOTE_UI}/{remote_name}", str(remote_copy),
        ], check=True, stdout=subprocess.DEVNULL)
        local = ROOT / local_name
        local_md5 = md5(local)
        remote_md5 = md5(remote_copy)
        same = local_md5 == remote_md5 and local.stat().st_size == remote_copy.stat().st_size
        print(f"{local_name}: same={same} bytes={local.stat().st_size} md5={local_md5}")
        if not same:
            raise RuntimeError(f"deploy mismatch: {local_name}")


if __name__ == "__main__":
    main()
