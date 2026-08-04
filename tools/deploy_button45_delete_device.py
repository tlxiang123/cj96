#!/usr/bin/env python3
"""Deploy and verify Button45 delete-device UI and logic."""

from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
PICTURE = "button45_delete_120x60.png"
FILES = (
    (ROOT / "ui" / "main.ftu", "/mnt/extsd/ui/main.ftu"),
    (ROOT / "Release" / "libzkgui.so", "/mnt/extsd/lib/libzkgui.so"),
    (ROOT / "ui" / PICTURE, f"/mnt/extsd/ui/{PICTURE}"),
    (ROOT / "resources" / PICTURE, f"/mnt/extsd/resources/{PICTURE}"),
)


def adb(*args: str, capture: bool = False) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(ADB), "-s", SERIAL, *args],
        check=True,
        text=True,
        capture_output=capture,
    )


def md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def main() -> None:
    missing = [str(local) for local, _ in FILES if not local.is_file()]
    if missing:
        raise FileNotFoundError("missing deployment files: " + ", ".join(missing))

    adb("connect", SERIAL, capture=True)
    adb("shell", "setprop", "ctl.stop", "zkswe")
    try:
        adb(
            "shell",
            "mkdir",
            "-p",
            "/mnt/extsd/ui",
            "/mnt/extsd/resources",
            "/mnt/extsd/lib",
        )
        for local, remote in FILES:
            adb("push", str(local), remote)
    finally:
        adb("shell", "setprop", "ctl.start", "zkswe")

    time.sleep(2)
    with tempfile.TemporaryDirectory(prefix="cj96_button45_verify_") as temp_dir:
        verify_dir = Path(temp_dir)
        for index, (local, remote) in enumerate(FILES):
            pulled = verify_dir / f"{index}_{local.name}"
            adb("pull", remote, str(pulled), capture=True)
            pulled_md5 = md5(pulled)
            print(f"{remote}: {pulled_md5} match={pulled_md5 == md5(local)}")

    state = adb("shell", "getprop", "init.svc.zkswe", capture=True).stdout.strip()
    processes = adb("shell", "ps", capture=True).stdout.splitlines()
    gui = next((line.strip() for line in processes if "/bin/zkgui" in line), "NOT_FOUND")
    print(f"zkswe={state}")
    print(f"gui_process={gui}")


if __name__ == "__main__":
    main()
