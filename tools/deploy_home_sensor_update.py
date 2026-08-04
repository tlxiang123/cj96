#!/usr/bin/env python3
from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"


FILES = (
    (ROOT / "Release" / "libzkgui.so", "/mnt/extsd/lib/libzkgui.so"),
    (ROOT / "ui" / "main.ftu", "/mnt/extsd/ui/main.ftu"),
    (ROOT / "resources" / "filter_icon_48.png", "/mnt/extsd/resources/filter_icon_48.png"),
    (ROOT / "resources" / "water_icon_48.png", "/mnt/extsd/resources/water_icon_48.png"),
    (ROOT / "ui" / "filter_icon_48.png", "/mnt/extsd/ui/filter_icon_48.png"),
    (ROOT / "ui" / "water_icon_48.png", "/mnt/extsd/ui/water_icon_48.png"),
)


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
    adb("shell", "mkdir", "-p", "/mnt/extsd/lib", "/mnt/extsd/ui", "/mnt/extsd/resources")
    adb("shell", "setprop", "ctl.stop", "zkswe")

    try:
        for local, remote in FILES:
            if not local.is_file():
                raise FileNotFoundError(local)
            adb("push", str(local), remote, capture=True)

        for frame in sorted((ROOT / "ui").glob("window7_pump_icon_anim_*.png")):
            adb("push", str(frame), f"/mnt/extsd/ui/{frame.name}", capture=True)
        for frame in sorted((ROOT / "resources").glob("window7_pump_icon_anim_*.png")):
            adb("push", str(frame), f"/mnt/extsd/resources/{frame.name}", capture=True)

        with tempfile.TemporaryDirectory(prefix="cj96_home_sensor_verify_") as temp:
            temp_dir = Path(temp)
            for local, remote in FILES:
                pulled = temp_dir / local.name
                adb("pull", remote, str(pulled), capture=True)
                if md5(local) != md5(pulled):
                    raise RuntimeError(f"md5 mismatch: {remote}")
                print(f"verified {remote} {md5(local)}")
    finally:
        adb("shell", "setprop", "ctl.start", "zkswe")

    print("deployed current main.ftu, resources, and libzkgui.so")


if __name__ == "__main__":
    main()
