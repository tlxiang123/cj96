#!/usr/bin/env python3
"""Initialize the CJ96 TF-card data layout on a connected board."""

from __future__ import annotations

import argparse
import subprocess
import tempfile
from pathlib import Path


DEFAULT_ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
DEFAULT_SERIAL = "192.168.1.70:5555"
DATA_DIRS = [
    "/mnt/extsd/cj96_data",
    "/mnt/extsd/cj96_data/config",
    "/mnt/extsd/cj96_data/logs",
    "/mnt/extsd/cj96_data/backup",
    "/mnt/extsd/cj96_tuya_demo",
    "/mnt/extsd/cj96_tuya_demo/inbox",
    "/mnt/extsd/cj96_tuya_demo/ack",
]


def run_adb(adb: Path, args: list[str], timeout: int = 20) -> str:
    result = subprocess.run(
        [str(adb), *args],
        text=True,
        encoding="utf-8",
        errors="replace",
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=timeout,
        check=False,
    )
    output = result.stdout.strip()
    if result.returncode != 0:
        raise RuntimeError(f"adb {' '.join(args)} failed:\n{output}")
    return output


def push_text(adb: Path, serial: str, remote_path: str, text: str) -> None:
    with tempfile.NamedTemporaryFile("w", encoding="utf-8", delete=False) as tmp:
        tmp.write(text)
        local = Path(tmp.name)
    try:
        run_adb(adb, ["-s", serial, "push", str(local), remote_path])
    finally:
        local.unlink(missing_ok=True)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adb", type=Path, default=DEFAULT_ADB)
    parser.add_argument("--serial", default=DEFAULT_SERIAL)
    args = parser.parse_args()

    if not args.adb.is_file():
        raise FileNotFoundError(f"adb not found: {args.adb}")

    print(run_adb(args.adb, ["connect", args.serial], timeout=10))
    print(run_adb(args.adb, ["-s", args.serial, "shell", "df", "/mnt/extsd"]))

    for directory in DATA_DIRS:
        run_adb(args.adb, ["-s", args.serial, "shell", "mkdir", "-p", directory])

    layout = "\n".join(
        [
            "CJ96 factory TF-card layout",
            "program files: /mnt/extsd/EasyUI.cfg /mnt/extsd/lib /mnt/extsd/ui /mnt/extsd/resources /mnt/extsd/font",
            "persistent config: /mnt/extsd/cj96_data/config",
            "persistent logs: /mnt/extsd/cj96_data/logs",
            "OTA runtime: /mnt/extsd/cj96_tuya_demo",
            "",
        ]
    )
    push_text(args.adb, args.serial, "/mnt/extsd/cj96_data/factory_layout.txt", layout)

    print(run_adb(args.adb, ["-s", args.serial, "shell", "ls", "-la", "/mnt/extsd/cj96_data"]))
    print(run_adb(args.adb, ["-s", args.serial, "shell", "ls", "-la", "/mnt/extsd/cj96_data/config"]))
    print(run_adb(args.adb, ["-s", args.serial, "shell", "ls", "-la", "/mnt/extsd/cj96_data/logs"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
