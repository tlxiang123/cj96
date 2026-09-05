#!/usr/bin/env python3
from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
LOCAL_LIB = ROOT / "Release" / "libzkgui.so"
REMOTE_LIB = "/mnt/extsd/lib/libzkgui.so"
REMOTE_TEMP_LIB = REMOTE_LIB + ".new"


def md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def adb(*args: str, capture: bool = False) -> subprocess.CompletedProcess:
    return subprocess.run(
        [str(ADB), "-s", SERIAL, *args],
        check=True,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.PIPE if capture else None,
    )


def remote_size(path: str) -> int:
    result = adb("shell", "ls", "-ln", path, capture=True)
    fields = result.stdout.split()
    if len(fields) < 4:
        raise RuntimeError(f"cannot read remote file size: {path}")
    try:
        return int(fields[3])
    except ValueError as error:
        raise RuntimeError(f"cannot read remote file size: {path}: {result.stdout.strip()}") from error


def main() -> None:
    if not LOCAL_LIB.is_file():
        print(f"SKIP missing {LOCAL_LIB}")
        return
    local_md5 = md5(LOCAL_LIB)
    adb("shell", "mkdir", "-p", "/mnt/extsd/lib")
    adb("shell", "rm", "-f", REMOTE_TEMP_LIB)
    adb("push", str(LOCAL_LIB), REMOTE_TEMP_LIB)
    expected_size = LOCAL_LIB.stat().st_size
    if remote_size(REMOTE_TEMP_LIB) != expected_size:
        raise RuntimeError("temporary library size verification failed")
    adb("shell", "mv", REMOTE_TEMP_LIB, REMOTE_LIB)
    if remote_size(REMOTE_LIB) != expected_size:
        raise RuntimeError("deployed library size verification failed")
    adb("shell", "setprop", "ctl.restart", "zkswe")
    print(f"PUSHED libzkgui.so local={local_md5}")


if __name__ == "__main__":
    main()
