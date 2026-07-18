#!/usr/bin/env python3
"""Deploy FTU and PNG resources to the connected cj96 target."""

from __future__ import annotations

import subprocess
import os
import sys
import json
import argparse
from pathlib import Path
from pathlib import PurePosixPath


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = os.environ.get("ADB_SERIAL", "192.168.1.103:5555")
REMOTE_UI = "/mnt/extsd/ui"
REMOTE_FONT = "/mnt/extsd/font"
REMOTE_CFG = "/mnt/extsd/EasyUI.cfg"


def adb(*args: str) -> None:
    subprocess.run([str(ADB), "-s", SERIAL, *args], check=True)


def adb_output(*args: str) -> bytes:
    return subprocess.check_output([str(ADB), "-s", SERIAL, *args])


def select_font_files(font_dir: Path) -> list[Path]:
    font = font_dir / "Alibaba-PuHuiTi-Regular.ttf"
    if not font.is_file():
        raise FileNotFoundError(f"required font is missing: {font}")
    return [font]


def update_easyui_font_config(font_files: list[Path]) -> None:
    if not font_files:
        return
    raw = adb_output("shell", "cat", REMOTE_CFG).decode("utf-8", errors="replace")
    config = json.loads(raw)
    config["font"] = ":".join(str(PurePosixPath(REMOTE_FONT) / path.name) for path in font_files)
    temp = ROOT / "Release" / "EasyUI.cfg"
    temp.write_text(json.dumps(config, ensure_ascii=False, indent=4), encoding="utf-8")
    adb("push", str(temp), REMOTE_CFG)


def main(skip_ftu_signature_check: bool = False) -> None:
    if not skip_ftu_signature_check:
        subprocess.run([sys.executable, str(ROOT / "tools" / "ftu_style.py"), "--verify"], cwd=ROOT, check=True)
    adb("shell", "setprop", "ctl.stop", "zkswe")
    try:
        library = ROOT / "Release" / "libzkgui.so"
        if library.exists():
            adb("push", str(library), "/mnt/extsd/lib/libzkgui.so")
        files = sorted((ROOT / "ui").glob("*.ftu"))
        for pattern in ("*.png", "*.jpg", "*.jpeg", "*.gif"):
            files += sorted((ROOT / "resources").rglob(pattern))
        created_dirs: set[str] = set()
        for path in files:
            if path.parent == ROOT / "ui":
                relative = PurePosixPath(path.name)
            else:
                relative = PurePosixPath(path.relative_to(ROOT / "resources").as_posix())
            destination = PurePosixPath(REMOTE_UI) / relative
            remote_parent = str(destination.parent)
            if remote_parent not in created_dirs:
                adb("shell", "mkdir", "-p", remote_parent)
                created_dirs.add(remote_parent)
            adb("push", str(path), str(destination))
        print(f"deployed {len(files)} UI files")
        font_dir = ROOT / "font"
        font_files = select_font_files(font_dir)
        if font_files:
            adb("shell", "mkdir", "-p", REMOTE_FONT)
            adb(
                "shell", "sh", "-c",
                f"rm -f {REMOTE_FONT}/*.ttf {REMOTE_FONT}/*.ttc {REMOTE_FONT}/*.otf",
            )
            for path in font_files:
                adb("push", str(path), str(PurePosixPath(REMOTE_FONT) / path.name))
            update_easyui_font_config(font_files)
            print(f"deployed {len(font_files)} font files")
    finally:
        adb("shell", "setprop", "ctl.start", "zkswe")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--skip-ftu-signature-check",
        action="store_true",
        help="deploy after a verified build when the saved UI signature predates new controls",
    )
    main(parser.parse_args().skip_ftu_signature_check)
