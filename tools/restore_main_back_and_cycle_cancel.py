#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Restore sys_back and CycleCancelButton after FlyThingsIDE saved stale canvas."""

from __future__ import annotations

import shutil
import sys
import time
from copy import deepcopy
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
RELEASE = ROOT / "Release"


def walk(node, path=""):
    if isinstance(node, dict):
        yield path, node
        for key, value in node.items():
            yield from walk(value, f"{path}/{key}")
    elif isinstance(node, list):
        for i, value in enumerate(node):
            yield from walk(value, f"{path}[{i}]")


def find_caption(node, caption: str):
    return [(p, n) for p, n in walk(node) if isinstance(n, dict) and n.get("caption") == caption]


def find_latest_good_backup():
    candidates = sorted(RELEASE.rglob("main.ftu"), key=lambda p: p.stat().st_mtime, reverse=True)
    for path in candidates:
        try:
            data, _, _ = decode_ftu(path)
        except Exception:
            continue
        if find_caption(data, "sys_back") and find_caption(data, "CycleCancelButton"):
            return path, data
    raise RuntimeError("no backup main.ftu contains both sys_back and CycleCancelButton")


def ensure_unique_key(parent: dict, preferred: str) -> str:
    if preferred not in parent:
        return preferred
    idx = 1
    while f"{preferred}_{idx}" in parent:
        idx += 1
    return f"{preferred}_{idx}"


def main() -> int:
    data, header, _ = decode_ftu(MAIN_FTU)
    backup_path, backup_data = find_latest_good_backup()
    changed = False

    stamp = time.strftime("%Y%m%d_%H%M%S")
    out_backup_dir = RELEASE / f"restore_back_cancel_from_backup_{stamp}"
    out_backup_dir.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, out_backup_dir / "main.before.ftu")
    shutil.copy2(backup_path, out_backup_dir / "source_with_controls.ftu")

    if not find_caption(data, "sys_back"):
        sys_back_hits = find_caption(backup_data, "sys_back")
        sys_back = deepcopy(sys_back_hits[0][1])
        data[ensure_unique_key(data, "button__sys_back")] = sys_back
        changed = True
        print(f"restored sys_back from {backup_path}")
    else:
        print("sys_back already exists")

    if not find_caption(data, "CycleCancelButton"):
        cycle_hits = find_caption(data, "CycleWindow")
        backup_cancel_hits = find_caption(backup_data, "CycleCancelButton")
        if not cycle_hits:
            raise RuntimeError("current CycleWindow not found")
        cancel = deepcopy(backup_cancel_hits[0][1])
        cycle = cycle_hits[0][1]
        cycle[ensure_unique_key(cycle, "button__cycle_cancel")] = cancel
        changed = True
        print(f"restored CycleCancelButton from {backup_path}")
    else:
        print("CycleCancelButton already exists")

    if changed:
        MAIN_FTU.write_bytes(encode_ftu(data, header))
        shutil.copy2(MAIN_FTU, out_backup_dir / "main.after.ftu")

    decoded, _, _ = decode_ftu(MAIN_FTU)
    for caption in ["sys_back", "CycleCancelButton"]:
        hits = find_caption(decoded, caption)
        if not hits:
            raise RuntimeError(f"{caption} still missing after restore")
        node = hits[0][1]
        print(f"OK {caption} id={node.get('id')} pos={node.get('position')} pic={node.get('picTab') or node.get('backgroundPic')}")

    print(f"backup_dir={out_backup_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
