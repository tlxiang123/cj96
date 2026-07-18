#!/usr/bin/env python3
"""Push the current UI with the known touch-stable GUI library."""

from pathlib import Path, PurePosixPath
import subprocess
import json

ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"


def adb(*args: str) -> None:
    subprocess.run([str(ADB), "-s", SERIAL, *args], check=True)


def main() -> None:
    adb("shell", "setprop", "ctl.stop", "zkswe")
    try:
        stable = ROOT / "backups" / "window2_smooth_no_grid_20260716_171911" / "Release" / "libzkgui.so"
        adb("push", str(stable), "/mnt/extsd/lib/libzkgui.so")
        files = sorted((ROOT / "ui").glob("*.ftu"))
        for pattern in ("*.png", "*.jpg", "*.jpeg", "*.gif"):
            files += sorted((ROOT / "resources").rglob(pattern))
        made: set[str] = set()
        for path in files:
            relative = PurePosixPath(path.name) if path.parent == ROOT / "ui" else PurePosixPath(path.relative_to(ROOT / "resources").as_posix())
            destination = PurePosixPath("/mnt/extsd/ui") / relative
            parent = str(destination.parent)
            if parent not in made:
                adb("shell", "mkdir", "-p", parent)
                made.add(parent)
            adb("push", str(path), str(destination))
        font = ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf"
        adb("shell", "mkdir", "-p", "/mnt/extsd/font")
        adb("push", str(font), "/mnt/extsd/font/Alibaba-PuHuiTi-Regular.ttf")
        cfg = json.loads(subprocess.check_output([str(ADB), "-s", SERIAL, "shell", "cat", "/mnt/extsd/EasyUI.cfg"]).decode("utf-8", errors="replace"))
        cfg["font"] = "/mnt/extsd/font/Alibaba-PuHuiTi-Regular.ttf"
        local_cfg = ROOT / "Release" / "EasyUI.cfg"
        local_cfg.write_text(json.dumps(cfg, ensure_ascii=False, indent=4), encoding="utf-8")
        adb("push", str(local_cfg), "/mnt/extsd/EasyUI.cfg")
        print(f"pushed stable library and {len(files)} UI assets")
    finally:
        adb("shell", "setprop", "ctl.start", "zkswe")


if __name__ == "__main__":
    main()
