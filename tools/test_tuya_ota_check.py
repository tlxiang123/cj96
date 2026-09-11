"""Trigger one CJ96 TuyaLink OTA check and print redacted diagnostics."""

from __future__ import annotations

import os
import re
import subprocess
import tempfile
import time
from pathlib import Path


ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
REMOTE_DIR = "/tmp/cj96_tuya_demo"


def adb(*args: str, timeout: int = 30) -> str:
    result = subprocess.run(
        [str(ADB), "-s", SERIAL, *args],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        errors="replace",
        timeout=timeout,
        check=False,
    )
    return result.stdout.strip()


def redact(line: str) -> str:
    line = re.sub(r"https?://[^\s\"']+", "[signed-url-redacted]", line)
    line = re.sub(
        r"(?i)(hmac|device_secret)\s*[:=]\s*[^,\s}\"']+",
        r"\1=[redacted]",
        line,
    )
    return line


def main() -> int:
    if not ADB.is_file():
        raise SystemExit(f"adb not found: {ADB}")

    print(f"CONNECT: {subprocess.run([str(ADB), 'connect', SERIAL], capture_output=True, text=True, check=False).stdout.strip()}")

    with tempfile.NamedTemporaryFile(delete=False) as request_file:
        request_file.write(b"check\n")
        request_path = request_file.name

    try:
        print(f"REQUEST_PUSH: {adb('push', request_path, f'{REMOTE_DIR}/ota_request')}")
    finally:
        os.unlink(request_path)

    time.sleep(10)

    request_left = adb("shell", "ls", "-l", f"{REMOTE_DIR}/ota_request")
    print(f"REQUEST_FILE_AFTER_10S: {request_left or '(consumed)'}")

    status = adb("shell", "cat", f"{REMOTE_DIR}/ota_status")
    if not status or "No such file" in status:
        status = "(not created)"
    print(f"OTA_STATUS: {status[:600]}")

    log = adb("shell", "cat", f"{REMOTE_DIR}/cj96_tuya_demo.run.log", timeout=30)
    keywords = (
        "ota",
        "upgrade",
        "firmware",
        "version",
        "connected",
        "request",
        "failed",
        "error",
    )
    matches = [
        redact(line)[:500]
        for line in log.splitlines()
        if any(keyword in line.lower() for keyword in keywords)
    ][-45:]

    print(f"LOG_MATCHES: {len(matches)}")
    print("\n".join(matches))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
