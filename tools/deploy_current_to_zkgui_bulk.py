#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import os
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = os.environ.get("ADB_SERIAL", "192.168.1.70:5555")
MAKE = Path(r"D:\Install\FlyThingsIDE\sdk\toolchains\t113\bin\make.exe")
TMP = PurePosixPath("/mnt/extsd/cj96_sync_tmp")


def run(cmd: list[str], *, check: bool = True, capture: bool = False) -> subprocess.CompletedProcess:
    env = os.environ.copy()
    env["PATH"] = str(MAKE.parent) + os.pathsep + env.get("PATH", "")
    result = subprocess.run(
        cmd,
        cwd=ROOT,
        env=env,
        check=False,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.STDOUT if capture else None,
    )
    if capture:
        print(">", " ".join(cmd))
        print(result.stdout or "")
    if check and result.returncode != 0:
        raise RuntimeError(f"command failed {result.returncode}: {' '.join(cmd)}\n{result.stdout or ''}")
    return result


def adb(*args: str, check: bool = True, capture: bool = False) -> subprocess.CompletedProcess:
    return run([str(ADB), "-s", SERIAL, *args], check=check, capture=capture)


def local_md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def remote_md5(path: str) -> str:
    result = adb("shell", "md5sum", path, check=False, capture=True)
    output = result.stdout or ""
    if result.returncode == 0 and "not found" not in output:
        parts = output.strip().split()
        if parts:
            return parts[0]
    with tempfile.TemporaryDirectory(prefix="cj96_remote_md5_") as temp:
        local_copy = Path(temp) / PurePosixPath(path).name
        adb("pull", path, str(local_copy))
        return local_md5(local_copy)


def service_state(name: str) -> str:
    result = adb("shell", "getprop", f"init.svc.{name}", check=False, capture=True)
    lines = [line.strip() for line in (result.stdout or "").splitlines() if line.strip()]
    return lines[-1] if lines else ""


def zkgui_pid() -> str:
    result = adb("shell", "ps", check=False, capture=True)
    for line in (result.stdout or "").splitlines():
        if "/bin/zkgui" in line:
            parts = line.split()
            return parts[1] if len(parts) > 1 else ""
    return ""


def build() -> None:
    run([str(MAKE), "-C", "Release", "all"])


def ensure_inputs() -> None:
    required = [
        ROOT / "Release" / "EasyUI.cfg",
        ROOT / "Release" / "libzkgui.so",
        ROOT / "ui" / "main.ftu",
        ROOT / "ui" / "cycle_window_background_1007x400.png",
        ROOT / "ui" / "w2_group_bind_cancel_120x60.png",
        ROOT / "resources",
        ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf",
    ]
    for path in required:
        if not path.exists():
            raise FileNotFoundError(path)


def connect() -> None:
    run([str(ADB), "connect", SERIAL], capture=True)
    adb("shell", "true", capture=True)


def upload_temp_tree() -> None:
    print(f"prepare temp {TMP}")
    adb("shell", "rm", "-rf", str(TMP))
    adb("shell", "mkdir", "-p", str(TMP / "lib"))
    adb("push", str(ROOT / "Release" / "EasyUI.cfg"), str(TMP / "EasyUI.cfg"))
    adb("push", str(ROOT / "Release" / "libzkgui.so"), str(TMP / "lib" / "libzkgui.so"))
    adb("push", str(ROOT / "ui"), str(TMP))
    adb("push", str(ROOT / "resources"), str(TMP))
    adb("push", str(ROOT / "font"), str(TMP))


def select_gui_service() -> tuple[str, str, str, str]:
    before_pid = zkgui_pid()
    zkgui = service_state("zkgui")
    zkswe = service_state("zkswe")
    service = "zkgui" if zkgui else "zkswe"
    print(f"before_pid={before_pid or '<empty>'} zkgui={zkgui or '<empty>'} zkswe={zkswe or '<empty>'} service={service}")
    return service, before_pid, zkgui, zkswe


def replace_remote_tree(service: str) -> None:
    adb("shell", "setprop", "ctl.stop", service)
    time.sleep(1)
    replace_cmd = (
        "set -e; "
        "rm -rf /mnt/extsd/ui /mnt/extsd/resources /mnt/extsd/font /mnt/extsd/lib /mnt/extsd/EasyUI.cfg; "
        "mv /mnt/extsd/cj96_sync_tmp/ui /mnt/extsd/ui; "
        "mv /mnt/extsd/cj96_sync_tmp/resources /mnt/extsd/resources; "
        "mv /mnt/extsd/cj96_sync_tmp/font /mnt/extsd/font; "
        "mv /mnt/extsd/cj96_sync_tmp/lib /mnt/extsd/lib; "
        "mv /mnt/extsd/cj96_sync_tmp/EasyUI.cfg /mnt/extsd/EasyUI.cfg; "
        "rmdir /mnt/extsd/cj96_sync_tmp"
    )
    try:
        adb("shell", "sh", "-c", replace_cmd)
    finally:
        adb("shell", "setprop", "ctl.start", service)
    time.sleep(2)
    print(f"after_pid={zkgui_pid() or '<empty>'}")


def verify() -> None:
    checks = [
        ("cfg", ROOT / "Release" / "EasyUI.cfg", "/mnt/extsd/EasyUI.cfg"),
        ("lib", ROOT / "Release" / "libzkgui.so", "/mnt/extsd/lib/libzkgui.so"),
        ("main_ftu", ROOT / "ui" / "main.ftu", "/mnt/extsd/ui/main.ftu"),
        (
            "cycle_bg",
            ROOT / "ui" / "cycle_window_background_1007x400.png",
            "/mnt/extsd/ui/cycle_window_background_1007x400.png",
        ),
        (
            "cancel_png",
            ROOT / "ui" / "w2_group_bind_cancel_120x60.png",
            "/mnt/extsd/ui/w2_group_bind_cancel_120x60.png",
        ),
        ("font", ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf", "/mnt/extsd/font/Alibaba-PuHuiTi-Regular.ttf"),
    ]
    for label, local, remote in checks:
        local_hash = local_md5(local)
        remote_hash = remote_md5(remote)
        status = "OK" if local_hash == remote_hash else "FAIL"
        print(f"{status} {label} local={local_hash} remote={remote_hash} remote_path={remote}")
        if local_hash != remote_hash:
            raise RuntimeError(f"{label} md5 mismatch")


def main() -> None:
    build()
    ensure_inputs()
    connect()
    upload_temp_tree()
    service, _, _, _ = select_gui_service()
    replace_remote_tree(service)
    verify()


if __name__ == "__main__":
    main()
