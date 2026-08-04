#!/usr/bin/env python3
from __future__ import annotations

import hashlib
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
FRAME_GLOB = "window7_pump_icon_anim_*.png"
REMOTE_DIRS = ("/mnt/extsd/ui", "/mnt/extsd/resources")


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
    files = sorted((ROOT / "resources").glob(FRAME_GLOB))
    if len(files) != 20:
        raise RuntimeError(f"expected 20 animation frames, found {len(files)}")

    adb("shell", "mkdir", "-p", *REMOTE_DIRS)
    for remote_dir in REMOTE_DIRS:
        for file in files:
            adb("push", str(file), f"{remote_dir}/{file.name}", capture=True)

    with tempfile.TemporaryDirectory(prefix="cj96_pump_anim_verify_") as temp:
        temp_dir = Path(temp)
        for remote_dir in REMOTE_DIRS:
            for file in files:
                pulled = temp_dir / f"{remote_dir.strip('/').replace('/', '_')}_{file.name}"
                adb("pull", f"{remote_dir}/{file.name}", str(pulled), capture=True)
                if md5(file) != md5(pulled):
                    raise RuntimeError(f"verify failed: {remote_dir}/{file.name}")

    print(f"deployed and verified {len(files)} frames to {', '.join(REMOTE_DIRS)}")


if __name__ == "__main__":
    main()
