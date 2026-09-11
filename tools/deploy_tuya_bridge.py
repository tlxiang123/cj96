#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
BRIDGE = ROOT / "integrations" / "tuya" / "bridge"
BUILD_SCRIPT = BRIDGE / "build.py"
BINARY = BRIDGE / "build" / "cj96_tuya_demo"
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
REMOTE_BINARY_DIR = "/tmp"
REMOTE_BINARY = f"{REMOTE_BINARY_DIR}/cj96_tuya_demo_bin"
REMOTE_CONFIG = "/data/cj96_tuya_demo.conf"


def run(
    command: list[str],
    *,
    cwd: Path = ROOT,
    check: bool = True,
    capture: bool = False,
) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        cwd=cwd,
        check=False,
        text=True,
        stdout=subprocess.PIPE if capture else None,
        stderr=subprocess.STDOUT if capture else None,
    )
    if capture and result.stdout:
        print(result.stdout.rstrip())
    if check and result.returncode != 0:
        raise RuntimeError(
            f"command failed ({result.returncode}): {' '.join(command)}\n"
            f"{result.stdout or ''}"
        )
    return result


def adb(serial: str, *args: str, check: bool = True, capture: bool = False) -> subprocess.CompletedProcess[str]:
    return run([str(ADB), "-s", serial, *args], check=check, capture=capture)


def md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest().upper()


def bridge_pids(serial: str) -> list[str]:
    result = adb(serial, "shell", "ps", check=False, capture=True)
    pids: list[str] = []
    for line in (result.stdout or "").splitlines():
        if REMOTE_BINARY not in line or " Z " in f" {line} ":
            continue
        fields = line.split()
        if len(fields) > 1 and fields[1].isdigit():
            pids.append(fields[1])
    return pids


def remote_file_exists(serial: str, path: str) -> bool:
    result = adb(serial, "shell", "ls", path, check=False, capture=True)
    output = result.stdout or ""
    return result.returncode == 0 and "No such file" not in output


def verify_remote_binary(serial: str) -> str:
    with tempfile.TemporaryDirectory(prefix="cj96_tuya_verify_") as temp:
        pulled = Path(temp) / "cj96_tuya_demo"
        adb(serial, "pull", REMOTE_BINARY, str(pulled))
        remote_hash = md5(pulled)
    local_hash = md5(BINARY)
    if remote_hash != local_hash:
        raise RuntimeError(f"binary MD5 mismatch: local={local_hash} board={remote_hash}")
    return local_hash


def deploy(serial: str, build: bool) -> None:
    if not ADB.exists():
        raise FileNotFoundError(ADB)

    if build:
        run([sys.executable, str(BUILD_SCRIPT)], cwd=BRIDGE)
    if not BINARY.exists():
        raise FileNotFoundError(BINARY)

    run([str(ADB), "connect", serial], check=False, capture=True)
    adb(serial, "shell", "true", capture=True)
    if not remote_file_exists(serial, REMOTE_CONFIG):
        raise FileNotFoundError(
            f"board config is missing: {REMOTE_CONFIG}; keep device credentials on the board"
        )

    timestamp = time.strftime("%Y%m%d_%H%M%S")
    remote_new = f"{REMOTE_BINARY}.new"
    remote_backup = f"{REMOTE_BINARY}.bak_{timestamp}"

    adb(serial, "push", str(BINARY), remote_new)
    adb(serial, "shell", "chmod", "755", remote_new)

    old_pids = bridge_pids(serial)
    for pid in old_pids:
        adb(serial, "shell", "kill", pid, check=False)
    time.sleep(1)
    remaining_pids = bridge_pids(serial)
    if remaining_pids:
        raise RuntimeError(f"bridge did not stop cleanly: {remaining_pids}")

    if remote_file_exists(serial, REMOTE_BINARY):
        adb(serial, "shell", "mv", REMOTE_BINARY, remote_backup)
    adb(serial, "shell", "mv", remote_new, REMOTE_BINARY)
    adb(serial, "shell", "chmod", "755", REMOTE_BINARY)

    try:
        adb(serial, "shell", REMOTE_BINARY, REMOTE_CONFIG)
        time.sleep(3)
        current_pids = bridge_pids(serial)
        if len(current_pids) != 1:
            raise RuntimeError(f"expected one bridge process, found {current_pids}")
        binary_hash = verify_remote_binary(serial)
    except Exception:
        adb(serial, "shell", "mv", REMOTE_BINARY, f"{REMOTE_BINARY}.failed_{timestamp}", check=False)
        if remote_file_exists(serial, remote_backup):
            adb(serial, "shell", "mv", remote_backup, REMOTE_BINARY, check=False)
            adb(serial, "shell", "chmod", "755", REMOTE_BINARY, check=False)
            adb(serial, "shell", REMOTE_BINARY, REMOTE_CONFIG, check=False)
        raise

    print(f"deployed serial={serial} pid={current_pids[0]} md5={binary_hash}")
    print(f"backup={remote_backup}")


def main() -> None:
    parser = argparse.ArgumentParser(description="Build and deploy the CJ96 Tuya MQTT bridge.")
    parser.add_argument(
        "--serial",
        required=True,
        help="Current board ADB address, for example 192.168.1.70:5555",
    )
    parser.add_argument("--no-build", action="store_true", help="Deploy the existing binary without rebuilding.")
    args = parser.parse_args()
    deploy(args.serial, build=not args.no_build)


if __name__ == "__main__":
    main()
