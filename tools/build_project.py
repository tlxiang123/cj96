#!/usr/bin/env python3
"""Build cj96 with the PATH expected by FlyThings-generated makefiles."""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RELEASE = ROOT / "Release"
GO = Path(r"D:\Install\Go\bin\go.exe")
TOOLCHAIN = Path(r"D:\Install\FlyThingsIDE\sdk\toolchains\t113\bin")
MAKE = TOOLCHAIN / "make.exe"
BRIDGE = ROOT / "integrations" / "tuya" / "bridge"
BRIDGE_BINARY = BRIDGE / "build" / "cj96_tuya_demo"
RUNTIME_BRIDGE = ROOT / "runtime" / "bin" / "cj96_tuya_demo"


def main() -> None:
    subprocess.run([sys.executable, str(BRIDGE / "build.py")], cwd=BRIDGE, check=True)
    RUNTIME_BRIDGE.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(BRIDGE_BINARY, RUNTIME_BRIDGE)
    for name in ("echo", "rm"):
        subprocess.run(
            [str(GO), "build", "-o", str(RELEASE / f"{name}.exe"), str(ROOT / "tools" / f"{name}.go")],
            check=True,
        )
    env = os.environ.copy()
    env["PATH"] = os.pathsep.join((str(RELEASE), str(TOOLCHAIN), env.get("PATH", "")))
    subprocess.run([str(MAKE), "all"], cwd=RELEASE, env=env, check=True)
    subprocess.run([sys.executable, str(ROOT / "tools" / "ftu_style.py"), "--verify"], cwd=ROOT, check=True)


if __name__ == "__main__":
    main()
