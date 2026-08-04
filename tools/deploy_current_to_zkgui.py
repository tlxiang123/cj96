#!/usr/bin/env python3
from __future__ import annotations

import hashlib
import os
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = os.environ.get("ADB_SERIAL", "192.168.1.70:5555")
MAKE = Path(r"D:\Install\FlyThingsIDE\sdk\toolchains\t113\bin\make.exe")
TOOLCHAIN_BIN = MAKE.parent

REMOTE_CFG = PurePosixPath("/mnt/extsd/EasyUI.cfg")
REMOTE_LIB_DIR = PurePosixPath("/mnt/extsd/lib")
REMOTE_UI_DIR = PurePosixPath("/mnt/extsd/ui")
REMOTE_RESOURCES_DIR = PurePosixPath("/mnt/extsd/resources")
REMOTE_FONT_DIR = PurePosixPath("/mnt/extsd/font")


def run(cmd: list[str], *, capture: bool = False) -> subprocess.CompletedProcess:
    env = os.environ.copy()
    env["PATH"] = str(TOOLCHAIN_BIN) + os.pathsep + env.get("PATH", "")
    return subprocess.run(
        cmd,
        cwd=ROOT,
        check=True,
        text=True,
        env=env,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.PIPE if capture else None,
    )


def adb(*args: str, capture: bool = False) -> subprocess.CompletedProcess:
    return run([str(ADB), "-s", SERIAL, *args], capture=capture)


def md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def ensure_remote_dir(path: PurePosixPath) -> None:
    adb("shell", "mkdir", "-p", str(path))


def push_file(local: Path, remote: PurePosixPath) -> None:
    ensure_remote_dir(remote.parent)
    adb("push", str(local), str(remote))


def remote_md5(remote: PurePosixPath) -> str:
    with tempfile.TemporaryDirectory(prefix="cj96_remote_md5_") as temp:
        local = Path(temp) / remote.name
        adb("pull", str(remote), str(local), capture=True)
        return md5(local)


def list_files(root: Path, suffixes: tuple[str, ...] | None = None) -> list[Path]:
    files: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        if suffixes is not None and path.suffix.lower() not in suffixes:
            continue
        files.append(path)
    return sorted(files)


def service_state(name: str) -> str:
    result = subprocess.run(
        [str(ADB), "-s", SERIAL, "shell", "getprop", f"init.svc.{name}"],
        cwd=ROOT,
        check=False,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return result.stdout.strip()


def process_lines() -> list[str]:
    result = adb("shell", "ps", capture=True)
    return [
        line.strip()
        for line in result.stdout.splitlines()
        if "zkgui" in line or "zkswe" in line
    ]


def zkgui_pid() -> str:
    lines = process_lines()
    for line in lines:
        if "/bin/zkgui" in line:
            parts = line.split()
            return parts[1] if len(parts) > 1 else ""
    return ""


def select_gui_service() -> str:
    zkgui_state = service_state("zkgui")
    zkswe_state = service_state("zkswe")
    if zkgui_state:
        return "zkgui"
    if zkswe_state or zkgui_pid():
        return "zkswe"
    return "zkgui"


def restart_gui_service(service: str, previous_pid: str) -> None:
    adb("shell", "setprop", "ctl.stop", service)
    adb("shell", "setprop", "ctl.start", service)
    for _ in range(30):
        current_pid = zkgui_pid()
        if current_pid and current_pid != previous_pid:
            print(f"restarted {service}: zkgui pid {previous_pid or '<none>'}->{current_pid}")
            return
    current_pid = zkgui_pid()
    raise RuntimeError(f"{service} restart did not refresh /bin/zkgui pid: before={previous_pid} after={current_pid}")


def build() -> None:
    if not MAKE.is_file():
        raise FileNotFoundError(MAKE)
    run([str(MAKE), "-C", "Release", "all"])


def deploy() -> dict[str, str | int]:
    local_cfg = ROOT / "Release" / "EasyUI.cfg"
    local_lib = ROOT / "Release" / "libzkgui.so"
    local_font = ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf"
    for required in (local_cfg, local_lib, local_font):
        if not required.is_file():
            raise FileNotFoundError(required)

    ui_files = list_files(ROOT / "ui")
    resource_files = list_files(ROOT / "resources", (".png", ".jpg", ".jpeg", ".gif"))
    font_files = list_files(ROOT / "font", (".ttf", ".ttc", ".otf"))

    before_zkgui = service_state("zkgui")
    before_zkswe = service_state("zkswe")
    before_pid = zkgui_pid()
    gui_service = select_gui_service()
    print(f"before init.svc.zkgui={before_zkgui or '<empty>'}")
    print(f"before init.svc.zkswe={before_zkswe or '<empty>'}")
    print(f"selected gui service={gui_service}")
    for line in process_lines():
        print(f"process {line}")

    adb("shell", "setprop", "ctl.stop", gui_service)
    try:
        push_file(local_cfg, REMOTE_CFG)
        push_file(local_lib, REMOTE_LIB_DIR / "libzkgui.so")

        for path in ui_files:
            remote = REMOTE_UI_DIR / PurePosixPath(path.relative_to(ROOT / "ui").as_posix())
            push_file(path, remote)

        for path in resource_files:
            relative = PurePosixPath(path.relative_to(ROOT / "resources").as_posix())
            push_file(path, REMOTE_RESOURCES_DIR / relative)
            push_file(path, REMOTE_UI_DIR / relative)

        for path in font_files:
            remote = REMOTE_FONT_DIR / PurePosixPath(path.relative_to(ROOT / "font").as_posix())
            push_file(path, remote)
    finally:
        adb("shell", "setprop", "ctl.start", gui_service)

    checks = {
        "cfg": (local_cfg, REMOTE_CFG),
        "lib": (local_lib, REMOTE_LIB_DIR / "libzkgui.so"),
        "main_ftu": (ROOT / "ui" / "main.ftu", REMOTE_UI_DIR / "main.ftu"),
        "font": (local_font, REMOTE_FONT_DIR / "Alibaba-PuHuiTi-Regular.ttf"),
        "resource_ui": (ROOT / "resources" / "Open_Valve.png", REMOTE_UI_DIR / "Open_Valve.png"),
        "resource_dir": (ROOT / "resources" / "Open_Valve.png", REMOTE_RESOURCES_DIR / "Open_Valve.png"),
    }
    for label, (local, remote) in checks.items():
        local_hash = md5(local)
        remote_hash = remote_md5(remote)
        status = "OK" if local_hash == remote_hash else "FAIL"
        print(f"{status} {label} local={local_hash} remote={remote_hash} remote_path={remote}")
        if local_hash != remote_hash:
            raise RuntimeError(f"{label} md5 mismatch")

    after_zkgui = service_state("zkgui")
    after_zkswe = service_state("zkswe")
    after_pid = zkgui_pid()
    if before_pid and after_pid == before_pid:
        restart_gui_service(gui_service, before_pid)
        after_pid = zkgui_pid()
        after_zkgui = service_state("zkgui")
        after_zkswe = service_state("zkswe")
    print(f"after init.svc.zkgui={after_zkgui or '<empty>'}")
    print(f"after init.svc.zkswe={after_zkswe or '<empty>'}")
    print(f"after zkgui_pid={after_pid or '<empty>'}")
    for line in process_lines():
        print(f"process {line}")

    return {
        "ui_files": len(ui_files),
        "resource_files": len(resource_files),
        "font_files": len(font_files),
        "zkgui_before": before_zkgui,
        "zkgui_after": after_zkgui,
    }


def main() -> None:
    build()
    summary = deploy()
    print(
        "deployed "
        f"ui={summary['ui_files']} "
        f"resources={summary['resource_files']} "
        f"fonts={summary['font_files']}"
    )


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        raise
