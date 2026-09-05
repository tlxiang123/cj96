#!/usr/bin/env python3
"""Create the desktop shortcut for the current CJ96 direct deployment entry point."""

from pathlib import Path
import sys

import win32com.client


ROOT = Path(__file__).resolve().parents[1]
SHORTCUT = Path.home() / "Desktop" / "CJ96一键烧录.lnk"
ENTRY_POINT = ROOT / "tools" / "CJ96_OneClickDeploy.py"
IDE = Path(r"D:\Install\FlyThingsIDE\bin\ide.exe")


def main() -> None:
    shell = win32com.client.Dispatch("WScript.Shell")
    shortcut = shell.CreateShortcut(str(SHORTCUT))
    shortcut.TargetPath = sys.executable
    shortcut.Arguments = f'"{ENTRY_POINT}"'
    shortcut.WorkingDirectory = str(ROOT)
    shortcut.Description = "构建并直推当前 CJ96 UI 到板子"
    if IDE.exists():
        shortcut.IconLocation = f"{IDE},0"
    shortcut.WindowStyle = 1
    shortcut.Save()

    check = shell.CreateShortcut(str(SHORTCUT))
    print(f"shortcut={SHORTCUT}")
    print(f"target={check.TargetPath}")
    print(f"arguments={check.Arguments}")
    print(f"workdir={check.WorkingDirectory}")


if __name__ == "__main__":
    main()
