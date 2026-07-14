#!/usr/bin/env python3
"""Build cj96 with the PATH expected by FlyThings-generated makefiles."""

from __future__ import annotations

import os
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RELEASE = ROOT / "Release"
GO = Path(r"D:\Install\Go\bin\go.exe")
TOOLCHAIN = Path(r"D:\Install\FlyThingsIDE\sdk\toolchains\t113\bin")
MAKE = TOOLCHAIN / "make.exe"


def main() -> None:
    for name in ("echo", "rm"):
        subprocess.run(
            [str(GO), "build", "-o", str(RELEASE / f"{name}.exe"), str(ROOT / "tools" / f"{name}.go")],
            check=True,
        )
    env = os.environ.copy()
    env["PATH"] = os.pathsep.join((str(RELEASE), str(TOOLCHAIN), env.get("PATH", "")))
    subprocess.run([str(MAKE), "all"], cwd=RELEASE, env=env, check=True)


if __name__ == "__main__":
    main()
