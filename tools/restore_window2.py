#!/usr/bin/env python3
"""Restore Window2 from the Git layout without replacing other current pages."""

from __future__ import annotations

import shutil
from pathlib import Path

from ftu_style import control_signature, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
CURRENT_PATH = ROOT / "ui" / "main.ftu"
GIT_PATH = ROOT / "Release" / "git_head_ui" / "ui" / "main.ftu"
BACKUP_PATH = ROOT / "ui" / "main.ftu.before-window2-merge-20260716.bak"


def main() -> None:
    current, current_header, _ = decode_ftu(CURRENT_PATH)
    git_layout, _, _ = decode_ftu(GIT_PATH)

    source_key = next(
        key
        for key, value in git_layout.items()
        if isinstance(value, dict) and value.get("caption") == "Window2"
    )
    source_window = git_layout[source_key]

    current = {
        key: value
        for key, value in current.items()
        if not (isinstance(value, dict) and value.get("caption") == "Window2")
    }

    merged: dict = {}
    inserted = False
    for key, value in current.items():
        if not inserted and isinstance(value, dict) and value.get("caption") == "Window3":
            merged[source_key] = source_window
            inserted = True
        merged[key] = value
    if not inserted:
        merged[source_key] = source_window

    shutil.copy2(CURRENT_PATH, BACKUP_PATH)
    CURRENT_PATH.write_bytes(encode_ftu(merged, current_header))

    verified, _, _ = decode_ftu(CURRENT_PATH)
    restored = [
        value
        for value in verified.values()
        if isinstance(value, dict) and value.get("caption") == "Window2"
    ]
    if len(restored) != 1 or len(control_signature(restored[0])) != 17:
        shutil.copy2(BACKUP_PATH, CURRENT_PATH)
        raise RuntimeError("Window2 verification failed; restored the backup")

    print(f"restored Window2 with {len(control_signature(restored[0]))} controls")
    print(f"backup: {BACKUP_PATH}")


if __name__ == "__main__":
    main()
