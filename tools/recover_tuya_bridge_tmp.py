#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
BINARY = ROOT / "integrations" / "tuya" / "bridge" / "build" / "cj96_tuya_demo"
CONFIG = Path(r"D:\code\tuya\cj96_tuya_demo\deploy\cj96_tuya_demo.conf")
REMOTE_BINARY = "/tmp/cj96_tuya_demo_bin"
REMOTE_CONFIG = "/data/cj96_tuya_demo.conf"
RUNTIME_DIR = "/tmp/cj96_tuya_demo"


def run(command: list[str], *, timeout: int = 30) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        command,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=timeout,
        check=False,
    )
    if result.stdout:
        print(result.stdout.rstrip())
    if result.stderr:
        print(result.stderr.rstrip())
    if result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}): {command}")
    return result


def adb(serial: str, *args: str, timeout: int = 30) -> subprocess.CompletedProcess[str]:
    return run([str(ADB), "-s", serial, *args], timeout=timeout)


def main() -> None:
    parser = argparse.ArgumentParser(description="Recover the CJ96 Tuya bridge without writing the SD card.")
    parser.add_argument("--serial", required=True)
    args = parser.parse_args()

    if not BINARY.exists():
        raise FileNotFoundError(BINARY)
    if not CONFIG.exists():
        raise FileNotFoundError(CONFIG)

    with tempfile.TemporaryDirectory(prefix="cj96_tuya_recover_") as temp_dir:
        daemon_config = Path(temp_dir) / "cj96_tuya_demo.conf"
        config_text = CONFIG.read_text(encoding="utf-8")
        config_lines = [
            "daemon=1" if line.startswith(("daemon=", "daemonize=")) else line
            for line in config_text.splitlines()
        ]
        daemon_config.write_text("\n".join(config_lines) + "\n", encoding="utf-8")

        adb(args.serial, "push", str(BINARY), REMOTE_BINARY)
        adb(args.serial, "push", str(daemon_config), REMOTE_CONFIG)

    adb(args.serial, "shell", "chmod", "755", REMOTE_BINARY)
    adb(args.serial, "shell", "chmod", "600", REMOTE_CONFIG)
    adb(args.serial, "shell", "rm", "-rf", RUNTIME_DIR)
    adb(args.serial, "shell", "mkdir", "-p", RUNTIME_DIR)
    adb(args.serial, "shell", REMOTE_BINARY, REMOTE_CONFIG)

    time.sleep(8)
    process_output = adb(args.serial, "shell", "ps").stdout
    bridge_lines = [line for line in process_output.splitlines() if REMOTE_BINARY in line]
    if len(bridge_lines) != 1:
        raise RuntimeError(f"expected one bridge process, found {bridge_lines}")

    run_log = adb(args.serial, "shell", "cat", f"{RUNTIME_DIR}/cj96_tuya_demo.run.log").stdout
    if "tuya_mqtt_connect ret=0" not in run_log or "connected product_id=" not in run_log:
        raise RuntimeError("bridge process is running but MQTT connection is not confirmed")

    print(bridge_lines[0])
    print("MQTT connection confirmed")


if __name__ == "__main__":
    main()
