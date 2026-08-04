#!/usr/bin/env python3
"""Deploy the system-time timezone selector changes to the cj96 target."""

from __future__ import annotations

import hashlib
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"

FILES = (
    (ROOT / "ui" / "showsysdate.ftu", "/mnt/extsd/ui/showsysdate.ftu"),
    (ROOT / "ui" / "timezone_select_190x60.png", "/mnt/extsd/ui/timezone_select_190x60.png"),
    (ROOT / "ui" / "timezone_dropdown_204x230.png", "/mnt/extsd/ui/timezone_dropdown_204x230.png"),
    (ROOT / "resources" / "timezone_select_190x60.png", "/mnt/extsd/resources/timezone_select_190x60.png"),
    (ROOT / "resources" / "timezone_dropdown_204x230.png", "/mnt/extsd/resources/timezone_dropdown_204x230.png"),
    (ROOT / "Release" / "libzkgui.so", "/mnt/extsd/lib/libzkgui.so"),
)


def adb(*args: str, capture: bool = False, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        [str(ADB), "-s", SERIAL, *args],
        check=check,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.PIPE if capture else None,
    )


def local_md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def remote_md5(path: str) -> str:
    with tempfile.TemporaryDirectory(prefix="cj96_timezone_verify_") as temp_dir:
        pulled = Path(temp_dir) / Path(path).name
        adb("pull", path, str(pulled), capture=True)
        return local_md5(pulled)


def zkgui_pid() -> str:
    result = adb("shell", "ps", capture=True, check=False)
    for line in result.stdout.splitlines():
        if "/bin/zkgui" in line:
            parts = line.split()
            return parts[1] if len(parts) > 1 else ""
    return ""


def main() -> None:
    for local, _remote in FILES:
        if not local.is_file():
            raise FileNotFoundError(local)

    before_pid = zkgui_pid()
    print(f"before_pid={before_pid or '<empty>'}")
    adb("shell", "mkdir", "-p", "/mnt/extsd/ui", "/mnt/extsd/resources", "/mnt/extsd/lib")
    adb("shell", "setprop", "ctl.stop", "zkswe")
    try:
        for local, remote in FILES:
            adb("push", str(local), remote)
    finally:
        adb("shell", "setprop", "ctl.start", "zkswe")

    after_pid = ""
    for _ in range(10):
        after_pid = zkgui_pid()
        if after_pid and after_pid != before_pid:
            break
    print(f"after_pid={after_pid or '<empty>'}")

    for local, remote in FILES:
        l_md5 = local_md5(local)
        r_md5 = remote_md5(remote)
        status = "OK" if l_md5 == r_md5 else "MISMATCH"
        print(f"{status} {local.relative_to(ROOT)} {l_md5} {remote} {r_md5}")
        if status != "OK":
            raise RuntimeError(f"deploy md5 mismatch: {local} -> {remote}")


if __name__ == "__main__":
    main()
