"""Set or clear the temporary OTA version override on a CJ96 board."""

from __future__ import annotations

import argparse
import re
from pathlib import Path
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SERIAL = "192.168.1.70:5555"
VERSION_FILE = ROOT / "tools" / "ota_test_version.txt"
REMOTE_DIR = "/mnt/extsd/cj96_tuya_demo"
REMOTE_FILE = REMOTE_DIR + "/test_version"
RUNTIME_BRIDGE = "/tmp/cj96_tuya_demo_bin"
RUN_LOG = "/tmp/cj96_tuya_demo/cj96_tuya_demo.run.log"
OTA_RESIDUE_PATHS = (
    "/mnt/extsd/update.img",
    "/mnt/extsd/update.img.part",
    "/mnt/extsd/zkautoupgrade",
    "/mnt/extsd/cj96_tuya_demo/update.img",
    "/mnt/extsd/cj96_tuya_demo/update.img.part",
    "/mnt/extsd/cj96_tuya_demo/zkautoupgrade",
    "/mnt/extsd/cj96_tuya_demo/ota_cleanup_pending",
    "/mnt/extsd/cj96_tuya_demo/ota_target_version",
)
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")


def run_adb(adb: list[str], *arguments: str, timeout: int = 20, check: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(
        [*adb, *arguments],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=timeout,
    )
    if check and result.returncode != 0:
        raise subprocess.CalledProcessError(
            result.returncode,
            [*adb, *arguments],
            output=result.stdout,
            stderr=result.stderr,
        )
    return result


def connect_board(serial: str) -> None:
    if ":" not in serial:
        return
    subprocess.run(
        [str(ADB), "connect", serial],
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        timeout=5,
        check=False,
    )


def write_test_version(adb: list[str], version: str) -> None:
    run_adb(adb, "shell", "mkdir", "-p", REMOTE_DIR)
    with tempfile.TemporaryDirectory(prefix="cj96_test_version_") as folder:
        local = Path(folder) / "test_version"
        local.write_text(version + "\n", encoding="ascii")
        temporary = REMOTE_FILE + ".new"
        run_adb(adb, "push", str(local), temporary)
        run_adb(adb, "shell", "mv", temporary, REMOTE_FILE)


def clear_test_version(adb: list[str]) -> None:
    run_adb(adb, "shell", "rm", "-f", REMOTE_FILE)


def clean_ota_residue(adb: list[str]) -> None:
    run_adb(adb, "shell", "rm", "-f", *OTA_RESIDUE_PATHS)
    run_adb(adb, "shell", "setprop", "sys.zkupgrade.force", "0", check=False)


def restart_zkswe(adb: list[str]) -> None:
    run_adb(adb, "shell", "setprop", "ctl.restart", "zkswe")
    for _ in range(20):
        time.sleep(1)
        result = run_adb(adb, "shell", "ps", check=False, timeout=8)
        if RUNTIME_BRIDGE in result.stdout:
            return
    raise TimeoutError("zkswe restarted, but Tuya bridge was not observed within 20 seconds")


def read_file(adb: list[str], path: str) -> str:
    result = run_adb(adb, "shell", "cat", path, check=False, timeout=8)
    return result.stdout.strip()


def print_version_log_summary(adb: list[str]) -> None:
    content = read_file(adb, RUN_LOG)
    interesting = [
        line
        for line in content.splitlines()
        if "OTA version report" in line
        or "OTA update completion" in line
        or "property report ack" in line
    ]
    if interesting:
        print("recent OTA log:")
        for line in interesting[-8:]:
            print("  " + line)


def read_version_file() -> str:
    if not VERSION_FILE.exists():
        VERSION_FILE.write_text("1.0.65\n", encoding="ascii")
    version = VERSION_FILE.read_text(encoding="ascii").strip()
    if not version:
        raise SystemExit(f"version file is empty: {VERSION_FILE}")
    return version


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--serial", default=DEFAULT_SERIAL)
    parser.add_argument("--restart-zkswe", action="store_true", help="restart zkswe so the bridge reports the new test version now")
    parser.add_argument("--clean-ota-residue", action="store_true", help="remove leftover OTA image/trigger/marker files before setting the test version")
    parser.add_argument("--clear", action="store_true", help="clear the test version override")
    parser.add_argument("version", nargs="?", help=f"Version such as 1.0.60; defaults to {VERSION_FILE}")
    args = parser.parse_args()

    if args.clear and args.version:
        raise SystemExit("use either --clear or a version, not both")
    version = None if args.clear else (args.version or read_version_file())
    if version and not re.fullmatch(r"\d{1,3}\.\d{1,3}\.\d{1,3}", version):
        raise SystemExit("version must look like 1.0.60")
    if not ADB.exists():
        raise FileNotFoundError(ADB)

    adb = [str(ADB), "-s", args.serial]
    connect_board(args.serial)
    if args.clean_ota_residue:
        clean_ota_residue(adb)
        print("OTA residue cleaned")
    if version:
        write_test_version(adb, version)
        print(f"test version set to {version}")
    else:
        clear_test_version(adb)
        print("test version override cleared")
    if args.restart_zkswe:
        restart_zkswe(adb)
        print("zkswe restarted")
        current = read_file(adb, REMOTE_FILE)
        if version and current != version:
            raise RuntimeError(f"test version file mismatch after restart: expected {version}, got {current!r}")
        print_version_log_summary(adb)
    elif version:
        current = read_file(adb, REMOTE_FILE)
        if current != version:
            raise RuntimeError(f"test version file mismatch: expected {version}, got {current!r}")


if __name__ == "__main__":
    main()
