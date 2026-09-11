"""Exercise version config parsing on T113 without starting MQTT or OTA."""

import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile
import uuid

from firmware_version import ROOT, read_firmware_version


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--serial", required=True)
    args = parser.parse_args()
    adb = [r"D:\Install\AndroidPlatformTools\adb.exe", "-s", args.serial]
    remote = "/tmp/cj96_version_check_" + uuid.uuid4().hex
    binary = ROOT / "runtime" / "bin" / "cj96_tuya_demo"

    def run(*arguments: str) -> str:
        result = subprocess.run(
            [*adb, *arguments], capture_output=True, text=True,
            encoding="utf-8", errors="replace", timeout=20, check=True,
        )
        return result.stdout.strip()

    with tempfile.TemporaryDirectory(prefix="cj96_version_test_") as folder:
        local = Path(folder)
        try:
            run("push", str(binary), remote)
            run("shell", "chmod", "755", remote)
            pulled = local / "bridge"
            run("pull", remote, str(pulled))
            assert hashlib.sha256(pulled.read_bytes()).digest() == hashlib.sha256(binary.read_bytes()).digest()
            for version in ("1.0.55", "9.9.99", None):
                config = local / "test.conf"
                config.write_text(
                    "device_id=version-test-only\ndevice_secret=not-a-real-secret\n"
                    + (f"firmware_version={version}\n" if version else ""),
                    encoding="ascii",
                )
                run("push", str(config), remote + ".conf")
                output = run("shell", remote, "--check-config", remote + ".conf")
                assert output == "firmware_version=" + read_firmware_version(), repr(output)
                print(f"PASS legacy_config={version or 'missing'} {output}")
            output = run("shell", remote, "--check-config", "/data/cj96_tuya_demo.conf")
            assert output == "firmware_version=" + read_firmware_version(), repr(output)
            print("PASS board_config " + output)
        finally:
            run("shell", "rm", "-f", remote, remote + ".conf")


if __name__ == "__main__":
    main()
