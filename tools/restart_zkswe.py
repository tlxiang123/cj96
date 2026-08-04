#!/usr/bin/env python3
from __future__ import annotations

import subprocess
from pathlib import Path


ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"


def main() -> None:
    subprocess.run([str(ADB), "-s", SERIAL, "shell", "setprop", "ctl.stop", "zkswe"], check=True)
    subprocess.run([str(ADB), "-s", SERIAL, "shell", "setprop", "ctl.start", "zkswe"], check=True)
    print("restarted zkswe")


if __name__ == "__main__":
    main()
