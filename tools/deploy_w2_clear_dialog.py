#!/usr/bin/env python3
"""Deploy the W2 clear-scope dialog files and verify the target copies."""

from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
FILES = (
    (ROOT / "ui" / "main.ftu", "/mnt/extsd/ui/main.ftu"),
    (ROOT / "ui" / "w2_clear_current_120x60.png", "/mnt/extsd/ui/w2_clear_current_120x60.png"),
    (ROOT / "ui" / "w2_clear_all_120x60.png", "/mnt/extsd/ui/w2_clear_all_120x60.png"),
    (
        ROOT / "resources" / "w2_clear_current_120x60.png",
        "/mnt/extsd/resources/w2_clear_current_120x60.png",
    ),
    (
        ROOT / "resources" / "w2_clear_all_120x60.png",
        "/mnt/extsd/resources/w2_clear_all_120x60.png",
    ),
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

    adb("shell", "setprop", "ctl.stop", "zkswe")
    try:
        adb("shell", "mkdir", "-p", "/mnt/extsd/ui", "/mnt/extsd/resources")
        for local, remote in FILES:
            adb("push", str(local), remote)
    finally:
        adb("shell", "setprop", "ctl.start", "zkswe")

    time.sleep(2)
    with tempfile.TemporaryDirectory(prefix="cj96_w2_clear_verify_") as temp_dir:
        verify_dir = Path(temp_dir)
        for index, (local, remote) in enumerate(FILES):
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
