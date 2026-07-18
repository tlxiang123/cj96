#!/usr/bin/env python3
"""Fast incremental UI deployment for the cj96 target.

The full IDE upload sends every resource file. This script only sends files
explicitly listed on the command line, which avoids thousands of ADB calls.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path, PurePosixPath


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
REMOTE_UI = PurePosixPath("/mnt/extsd/ui")
MANIFEST = ROOT / "Release" / ".incremental_ui_manifest.json"


def adb(*args: str) -> None:
    subprocess.run([str(ADB), "-s", SERIAL, *args], check=True)


def digest(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def remote_path(path: Path) -> PurePosixPath:
    if path.parent == ROOT / "ui":
        return REMOTE_UI / path.name
    return REMOTE_UI / PurePosixPath(path.relative_to(ROOT / "resources").as_posix())


def load_manifest() -> dict[str, str]:
    if not MANIFEST.exists():
        return {}
    return json.loads(MANIFEST.read_text(encoding="utf-8"))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "files", nargs="+", help="local files, for example ui/main.ftu resources/window8_panel.png"
    )
    parser.add_argument("--restart", action="store_true", help="restart zkswe after upload")
    args = parser.parse_args()

    manifest = load_manifest()
    changed: list[tuple[Path, PurePosixPath, str]] = []
    for name in args.files:
        path = (ROOT / name).resolve()
        if not path.is_file():
            raise FileNotFoundError(path)
        if path.parent not in (ROOT / "ui", ROOT / "resources") and ROOT / "resources" not in path.parents:
            raise ValueError(f"file is outside ui/resources: {path}")
        md5 = digest(path)
        key = str(path.relative_to(ROOT)).replace("\\", "/")
        if manifest.get(key) != md5:
            changed.append((path, remote_path(path), md5))

    if not changed:
        print("no changed files to push")
        return

    if args.restart:
        adb("shell", "setprop", "ctl.stop", "zkswe")
    try:
        created: set[str] = set()
        for path, destination, md5 in changed:
            parent = str(destination.parent)
            if parent not in created:
                adb("shell", "mkdir", "-p", parent)
                created.add(parent)
            adb("push", str(path), str(destination))
            manifest[str(path.relative_to(ROOT)).replace("\\", "/")] = md5
    finally:
        if args.restart:
            adb("shell", "setprop", "ctl.start", "zkswe")

    MANIFEST.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"pushed {len(changed)} changed files")


if __name__ == "__main__":
    main()
