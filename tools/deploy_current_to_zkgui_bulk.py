#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import sys
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = os.environ.get("ADB_SERIAL", "192.168.1.70:5555")
MAKE = Path(r"D:\Install\FlyThingsIDE\sdk\toolchains\t113\bin\make.exe")
TMP = PurePosixPath(f"/mnt/extsd/cj96_sync_tmp_{os.getpid()}")
BRIDGE = ROOT / "integrations" / "tuya" / "bridge"
BRIDGE_BINARY = BRIDGE / "build" / "cj96_tuya_demo"
RUNTIME_BRIDGE = ROOT / "runtime" / "bin" / "cj96_tuya_demo"
REMOTE_UPDATE_IMAGE = "/mnt/extsd/update.img"
REMOTE_BRIDGE = "/mnt/extsd/tuya_demo/cj96_tuya_demo"
REMOTE_BRIDGE_CONFIG = "/mnt/extsd/tuya_demo/cj96_tuya_demo.conf"
INTERNAL_BRIDGE_CONFIG = "/data/cj96_tuya_demo.conf"


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
            return parts[0] if parts and parts[0].isdigit() else ""
    return ""


def tuya_bridge_pids() -> list[str]:
    result = adb("shell", "ps", check=False, capture=True)
    pids: list[str] = []
    for line in (result.stdout or "").splitlines():
        if "cj96_tuya_demo" not in line or " Z " in f" {line} ":
            continue
        parts = line.split()
        if parts and parts[0].isdigit():
            pids.append(parts[0])
    return pids


def wait_for_process(get_pids, *, timeout_seconds: int) -> list[str]:
    deadline = time.monotonic() + timeout_seconds
    while time.monotonic() < deadline:
        pids = get_pids()
        if pids:
            return pids
        time.sleep(1)
    return []


def remove_stale_update_image() -> None:
    adb("shell", "rm", "-f", REMOTE_UPDATE_IMAGE)
    result = adb("shell", "ls", REMOTE_UPDATE_IMAGE, check=False, capture=True)
    if result.returncode == 0 and "No such file" not in (result.stdout or ""):
        raise RuntimeError(f"failed to remove stale upgrade image: {REMOTE_UPDATE_IMAGE}")
    print(f"removed stale upgrade image: {REMOTE_UPDATE_IMAGE}")


def ensure_tuya_bridge_running() -> None:
    config_check = adb(
        "shell", "ls", "-l", INTERNAL_BRIDGE_CONFIG, check=False, capture=True
    )
    if config_check.returncode != 0:
        print(
            "Tuya bridge config is absent at "
            f"{INTERNAL_BRIDGE_CONFIG}; GUI deployment will continue."
        )
        return

    pids = wait_for_process(tuya_bridge_pids, timeout_seconds=8)
    if not pids:
        pids = wait_for_process(tuya_bridge_pids, timeout_seconds=8)
    if len(pids) != 1:
        raise RuntimeError(f"expected one Tuya bridge process, found {pids}")
    print(f"tuya_bridge_pid={pids[0]}")


def build() -> None:
    run([sys.executable, str(BRIDGE / "build.py")])
    RUNTIME_BRIDGE.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(BRIDGE_BINARY, RUNTIME_BRIDGE)
    run([str(MAKE), "-C", "Release", "all"])


def ensure_inputs() -> None:
    required = [
        ROOT / "Release" / "EasyUI.cfg",
        ROOT / "Release" / "libzkgui.so",
        ROOT / "runtime" / "bin" / "cj96_tuya_demo",
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


def assert_extsd_writable() -> None:
    result = adb(
        "shell",
        "sh",
        "-c",
        "echo cj96_write_test > /mnt/extsd/.cj96_write_test 2>&1 && "
        "rm -f /mnt/extsd/.cj96_write_test 2>&1",
        check=False,
        capture=True,
    )
    if result.returncode != 0 or "Read-only file system" in (result.stdout or ""):
        mounts = adb("shell", "cat", "/proc/mounts", check=False, capture=True).stdout or ""
        extsd_mount = ""
        for line in mounts.splitlines():
            if " /mnt/extsd " in line:
                extsd_mount = line
                break
        raise RuntimeError(
            "/mnt/extsd is read-only; deployment cannot write UI/lib files. "
            f"mount={extsd_mount or 'not found'}"
        )


def ensure_quick_run_storage() -> None:
    """Use internal /data for the IDE's temporary /mnt/extsd runtime."""
    mounts = adb("shell", "cat", "/proc/mounts", capture=True).stdout or ""
    if " /mnt/extsd " in mounts:
        return

    result = adb(
        "shell", "mount", "-o", "bind", "/data", "/mnt/extsd",
        check=False,
        capture=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            "TF card is not mounted and /data could not be bound to "
            f"/mnt/extsd: {result.stdout or '<no output>'}"
        )

    mounts = adb("shell", "cat", "/proc/mounts", capture=True).stdout or ""
    if " /mnt/extsd " not in mounts:
        raise RuntimeError("/mnt/extsd bind mount did not become active")
    print("quick-run storage: /mnt/extsd is bound to internal /data")


def upload_temp_tree() -> None:
    print(f"prepare temp {TMP}")
    adb("shell", "rm", "-rf", str(TMP))
    adb("shell", "mkdir", "-p", str(TMP / "lib"))
    adb("shell", "mkdir", "-p", str(TMP / "bin"))
    adb("push", str(ROOT / "Release" / "EasyUI.cfg"), str(TMP / "EasyUI.cfg"))
    adb("push", str(ROOT / "Release" / "libzkgui.so"), str(TMP / "lib" / "libzkgui.so"))
    adb(
        "push",
        str(ROOT / "runtime" / "bin" / "cj96_tuya_demo"),
        str(TMP / "bin" / "cj96_tuya_demo"),
    )
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
    for pid in tuya_bridge_pids():
        adb("shell", "kill", pid, check=False)
    time.sleep(1)
    replace_cmd = (
        "set -e; "
        "rm -rf /mnt/extsd/ui /mnt/extsd/resources /mnt/extsd/font /mnt/extsd/lib /mnt/extsd/EasyUI.cfg; "
        f"mv {TMP}/ui /mnt/extsd/ui; "
        f"mv {TMP}/resources /mnt/extsd/resources; "
        f"mv {TMP}/font /mnt/extsd/font; "
        f"mv {TMP}/lib /mnt/extsd/lib; "
        f"mv {TMP}/EasyUI.cfg /mnt/extsd/EasyUI.cfg; "
        "mkdir -p /mnt/extsd/tuya_demo; "
        f"mv {TMP}/bin/cj96_tuya_demo /mnt/extsd/tuya_demo/cj96_tuya_demo; "
        "chmod 755 /mnt/extsd/tuya_demo/cj96_tuya_demo; "
        f"rmdir {TMP}/bin; "
        f"rmdir {TMP}"
    )
    try:
        adb("shell", "sh", "-c", replace_cmd)
    finally:
        adb("shell", "setprop", "ctl.start", service)
    deadline = time.monotonic() + 10
    current_pid = ""
    while time.monotonic() < deadline:
        current_pid = zkgui_pid()
        if current_pid:
            break
        time.sleep(1)
    if not current_pid:
        raise RuntimeError("GUI process did not start after deployment")
    print(f"after_pid={current_pid}")
    ensure_tuya_bridge_running()


def verify() -> None:
    checks = [
        ("cfg", ROOT / "Release" / "EasyUI.cfg", "/mnt/extsd/EasyUI.cfg"),
        ("lib", ROOT / "Release" / "libzkgui.so", "/mnt/extsd/lib/libzkgui.so"),
        (
            "tuya_bridge",
            ROOT / "runtime" / "bin" / "cj96_tuya_demo",
            "/mnt/extsd/tuya_demo/cj96_tuya_demo",
        ),
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
    global SERIAL
    parser = argparse.ArgumentParser(description="Build and deploy the current CJ96 GUI tree.")
    parser.add_argument(
        "--serial",
        default=SERIAL,
        help="Current board ADB address, for example 192.168.1.70:5555",
    )
    args = parser.parse_args()
    SERIAL = args.serial
    ensure_inputs()
    connect()
    ensure_quick_run_storage()
    assert_extsd_writable()
    remove_stale_update_image()
    build()
    upload_temp_tree()
    service, _, _, _ = select_gui_service()
    replace_remote_tree(service)
    verify()


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"CJ96 deploy error: {exc}", file=sys.stderr)
        raise SystemExit(1)
