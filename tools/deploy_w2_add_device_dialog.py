#!/usr/bin/env python3
from __future__ import annotations

import hashlib
from pathlib import Path, PurePosixPath
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"

FILES = [
    (ROOT / "Release" / "libzkgui.so", PurePosixPath("/mnt/extsd/lib/libzkgui.so")),
    (ROOT / "ui" / "main.ftu", PurePosixPath("/mnt/extsd/ui/main.ftu")),
    (
        ROOT / "ui" / "w2_add_device_dialog_650x300.png",
        PurePosixPath("/mnt/extsd/ui/w2_add_device_dialog_650x300.png"),
    ),
    (
        ROOT / "resources" / "w2_add_device_dialog_650x300.png",
        PurePosixPath("/mnt/extsd/resources/w2_add_device_dialog_650x300.png"),
    ),
]


def run_adb(*args: str, check: bool = True, capture: bool = False) -> subprocess.CompletedProcess:
    result = subprocess.run(
        [str(ADB), "-s", SERIAL, *args],
        check=False,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.STDOUT if capture else None,
    )
    if capture and result.stdout:
        print(result.stdout, end="")
    if check and result.returncode != 0:
        raise RuntimeError(f"adb failed {result.returncode}: {' '.join(args)}\n{result.stdout or ''}")
    return result


def md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def remote_md5(remote: PurePosixPath) -> str:
    with tempfile.TemporaryDirectory(prefix="cj96_verify_w2_add_") as temp:
        local = Path(temp) / remote.name
        run_adb("pull", str(remote), str(local), capture=True)
        return md5(local)


def zkgui_lines() -> list[str]:
    result = run_adb("shell", "ps", check=False, capture=True)
    return [line for line in (result.stdout or "").splitlines() if "/bin/zkgui" in line]


def main() -> None:
    subprocess.run([str(ADB), "connect", SERIAL], check=False, text=True)
    for local, _remote in FILES:
        if not local.is_file():
            raise FileNotFoundError(local)

    print(f"before_zkgui={zkgui_lines()}")
    run_adb("shell", "setprop", "ctl.stop", "zkswe", check=False, capture=True)
    time.sleep(0.5)
    try:
        for local, remote in FILES:
            run_adb("shell", "mkdir", "-p", str(remote.parent), capture=True)
            run_adb("push", str(local), str(remote), capture=True)
    finally:
        run_adb("shell", "setprop", "ctl.start", "zkswe", check=False, capture=True)

    time.sleep(1.5)
    for local, remote in FILES:
        local_hash = md5(local)
        board_hash = remote_md5(remote)
        status = "OK" if local_hash == board_hash else "FAIL"
        print(f"{status} {local.relative_to(ROOT)} local={local_hash} remote={board_hash} remote_path={remote}")
        if local_hash != board_hash:
            raise RuntimeError(f"md5 mismatch: {remote}")
    print(f"after_zkgui={zkgui_lines()}")


if __name__ == "__main__":
    main()
