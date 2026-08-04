#!/usr/bin/env python3
from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
LOCAL_LIB = ROOT / "Release" / "libzkgui.so"
REMOTE_LIB = "/mnt/extsd/lib/libzkgui.so"


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


def main() -> None:
    if not LOCAL_LIB.is_file():
        print(f"SKIP missing {LOCAL_LIB}")
        return
    local_md5 = md5(LOCAL_LIB)
    with tempfile.TemporaryDirectory(prefix="cj96_verify_lib_") as temp:
        pulled = Path(temp) / "libzkgui.so"
        try:
            adb("pull", REMOTE_LIB, str(pulled), capture=True)
            remote_md5 = md5(pulled)
        except subprocess.CalledProcessError:
            remote_md5 = ""
        if local_md5 == remote_md5:
            print(f"OK libzkgui.so already same md5={local_md5}")
            return
    adb("shell", "setprop", "ctl.stop", "zkswe")
    try:
        adb("shell", "mkdir", "-p", "/mnt/extsd/lib")
        adb("push", str(LOCAL_LIB), REMOTE_LIB)
    finally:
        adb("shell", "setprop", "ctl.start", "zkswe")
    print(f"PUSHED libzkgui.so local={local_md5}")


if __name__ == "__main__":
    main()
