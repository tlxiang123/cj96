#!/usr/bin/env python3
"""Desktop entry point for building and deploying the complete CJ96 runtime."""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import time
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEPLOY_SCRIPT = ROOT / "tools" / "deploy_current_to_zkgui_bulk.py"
IDE = Path(r"D:\Install\FlyThingsIDE\bin\ide.exe")
ADB_CANDIDATES = (
    Path(r"D:\Install\FlyThingsIDE\sdk\platform-tools\adb\adb.exe"),
    Path(r"D:\Install\FlyThingsIDE\sdk\platform-tools\adb-1.0.32\adb.exe"),
)
NETWORK_TARGETS = ("192.168.1.70:5555", "192.168.1.69:5555")
SHORTCUT_NAME = "CJ96\u4e00\u952e\u70e7\u5f55.lnk"


def find_adb() -> str:
    installed = shutil.which("adb")
    if installed:
        return installed
    for candidate in ADB_CANDIDATES:
        if candidate.exists():
            return str(candidate)
    raise FileNotFoundError("ADB was not found")


def adb_devices(adb: str) -> list[str]:
    result = subprocess.run(
        [adb, "devices"],
        text=True,
        capture_output=True,
        timeout=10,
        check=True,
    )
    devices: list[str] = []
    for line in result.stdout.splitlines()[1:]:
        fields = line.split()
        if len(fields) >= 2 and fields[1] == "device":
            devices.append(fields[0])
    return devices


def choose_target(adb: str) -> str:
    subprocess.run([adb, "start-server"], timeout=10, check=True)

    online = adb_devices(adb)
    for preferred in NETWORK_TARGETS:
        if preferred in online:
            return preferred
    if len(online) == 1:
        return online[0]

    for target in NETWORK_TARGETS:
        for _ in range(3):
            subprocess.run(
                [adb, "connect", target],
                text=True,
                capture_output=True,
                timeout=10,
                check=False,
            )
            time.sleep(1)
            online = adb_devices(adb)
            if target in online:
                return target

    online = adb_devices(adb)
    if online:
        return online[0]
    raise RuntimeError(
        "No CJ96 board is online over USB, 192.168.1.69:5555, or 192.168.1.70:5555"
    )


def install_shortcut() -> Path:
    try:
        import win32com.client  # type: ignore
    except ImportError as exc:
        raise RuntimeError("pywin32 is required to create the desktop shortcut") from exc

    desktop = Path.home() / "Desktop"
    shortcut_path = desktop / SHORTCUT_NAME
    shortcut = win32com.client.Dispatch("WScript.Shell").CreateShortcut(
        str(shortcut_path)
    )
    shortcut.TargetPath = sys.executable
    shortcut.Arguments = f'"{Path(__file__).resolve()}"'
    shortcut.WorkingDirectory = str(ROOT)
    shortcut.Description = "Build and deploy the complete CJ96 runtime with Tuya MQTT"
    if IDE.exists():
        shortcut.IconLocation = f"{IDE},0"
    shortcut.WindowStyle = 1
    shortcut.Save()
    return shortcut_path


def wait_for_user() -> None:
    try:
        input("\nPress Enter to close...")
    except EOFError:
        pass


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--install-shortcut", action="store_true")
    parser.add_argument("--no-wait", action="store_true")
    args = parser.parse_args()

    if args.install_shortcut:
        print(install_shortcut())
        return 0

    exit_code = 0
    try:
        adb = find_adb()
        target = choose_target(adb)
        print(f"CJ96 target: {target}")
        print("Building and deploying GUI, resources, font, and Tuya MQTT bridge...")
        result = subprocess.run(
            [sys.executable, str(DEPLOY_SCRIPT), "--serial", target],
            cwd=ROOT,
        )
        if result.returncode != 0:
            raise RuntimeError("deployment script exited with an error; see the message above")
        print("\nCJ96 deployment completed successfully.")
    except Exception as exc:
        exit_code = 1
        print(f"\nCJ96 deployment failed: {exc}", file=sys.stderr)

    if not args.no_wait:
        wait_for_user()
    return exit_code


if __name__ == "__main__":
    raise SystemExit(main())
