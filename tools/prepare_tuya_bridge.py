#!/usr/bin/env python3
"""Build the Tuya bridge and refresh the GUI-embedded binary header."""

from __future__ import annotations

import shutil
import subprocess
import sys
from pathlib import Path
from firmware_version import sync_version_label


ROOT = Path(__file__).resolve().parents[1]
BRIDGE = ROOT / "integrations" / "tuya" / "bridge"
BRIDGE_BINARY = BRIDGE / "build" / "cj96_tuya_demo"
RUNTIME_BRIDGE = ROOT / "runtime" / "bin" / "cj96_tuya_demo"


def main() -> int:
    sync_version_label()
    subprocess.run([sys.executable, str(BRIDGE / "build.py")], cwd=BRIDGE, check=True)
    RUNTIME_BRIDGE.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(BRIDGE_BINARY, RUNTIME_BRIDGE)
    subprocess.run(
        [sys.executable, str(ROOT / "tools" / "embed_tuya_bridge.py")],
        cwd=ROOT,
        check=True,
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
