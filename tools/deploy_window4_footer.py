#!/usr/bin/env python3
"""Deploy and verify the Window4 fixed irrigation footer."""

from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
ASSETS = (
    "window4_region1_319x384.png",
    "window4_region2_319x384.png",
    "window4_region3_321x384.png",
    "window4_cycle_toggle_off_80x43.png",
    "window4_cycle_toggle_on_80x43.png",
    "cycle_edit_46x40.png",
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


def deployment_files() -> list[tuple[Path, str]]:
    files = [
        (ROOT / "ui" / "main.ftu", "/mnt/extsd/ui/main.ftu"),
        (ROOT / "Release" / "libzkgui.so", "/mnt/extsd/lib/libzkgui.so"),
    ]
    for directory in ("ui", "resources"):
        for name in ASSETS:
            files.append(
                (ROOT / directory / name, f"/mnt/extsd/{directory}/{name}")
            )
    return files


def main() -> None:
    files = deployment_files()
    missing = [str(local) for local, _ in files if not local.is_file()]
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
        for local, remote in files:
            adb("push", str(local), remote)
    finally:
        adb("shell", "setprop", "ctl.start", "zkswe")

    time.sleep(2)
    with tempfile.TemporaryDirectory(prefix="cj96_window4_verify_") as temp_dir:
        verify_dir = Path(temp_dir)
        for index, (local, remote) in enumerate(files):
            pulled = verify_dir / f"{index}_{local.name}"
            adb("pull", remote, str(pulled), capture=True)
            remote_md5 = md5(pulled)
            print(f"{remote}: {remote_md5} match={remote_md5 == md5(local)}")

    state = adb("shell", "getprop", "init.svc.zkswe", capture=True).stdout.strip()
    processes = adb("shell", "ps", capture=True).stdout.splitlines()
    gui = next((line.strip() for line in processes if "/bin/zkgui" in line), "NOT_FOUND")
    print(f"zkswe={state}")
    print(f"gui_process={gui}")


if __name__ == "__main__":
    main()
