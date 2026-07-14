#!/usr/bin/env python3
"""Deploy FTU and PNG resources to the connected cj96 target."""

from __future__ import annotations

import subprocess
import os
from pathlib import Path
from pathlib import PurePosixPath


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = os.environ.get("ADB_SERIAL", "192.168.1.103:5555")
REMOTE_UI = "/mnt/extsd/ui"


def adb(*args: str) -> None:
    subprocess.run([str(ADB), "-s", SERIAL, *args], check=True)


def main() -> None:
    adb("shell", "setprop", "ctl.stop", "zkswe")
    try:
        library = ROOT / "Release" / "libzkgui.so"
        if library.exists():
            adb("push", str(library), "/mnt/extsd/lib/libzkgui.so")
        files = sorted((ROOT / "ui").glob("*.ftu"))
        for pattern in ("*.png", "*.jpg", "*.jpeg", "*.gif"):
            files += sorted((ROOT / "resources").rglob(pattern))
        created_dirs: set[str] = set()
        for path in files:
            if path.parent == ROOT / "ui":
                relative = PurePosixPath(path.name)
            else:
                relative = PurePosixPath(path.relative_to(ROOT / "resources").as_posix())
            destination = PurePosixPath(REMOTE_UI) / relative
            remote_parent = str(destination.parent)
            if remote_parent not in created_dirs:
                adb("shell", "mkdir", "-p", remote_parent)
                created_dirs.add(remote_parent)
            adb("push", str(path), str(destination))
        print(f"deployed {len(files)} UI files")
    finally:
        adb("shell", "setprop", "ctl.start", "zkswe")


if __name__ == "__main__":
    main()
