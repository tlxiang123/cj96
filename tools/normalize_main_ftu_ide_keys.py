#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Rename non-standard FTU control keys to FlyThingsIDE-visible numeric keys.

FlyThingsIDE can preserve a control in the encoded JSON but fail to render it
on the canvas when the key is manually named like button__sys_back. Existing
IDE-created controls use keys such as button__184. This script keeps the same
control IDs/captions and only renames the container keys.
"""

from __future__ import annotations

import re
import shutil
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
RELEASE = ROOT / "Release"


def max_numeric_suffix(node: object, prefix: str) -> int:
    max_value = 0
    if isinstance(node, dict):
        for key, value in node.items():
            m = re.fullmatch(re.escape(prefix) + r"__(\d+)", key)
            if m:
                max_value = max(max_value, int(m.group(1)))
            max_value = max(max_value, max_numeric_suffix(value, prefix))
    elif isinstance(node, list):
        for value in node:
            max_value = max(max_value, max_numeric_suffix(value, prefix))
    return max_value


def next_key(root: dict, parent: dict, prefix: str) -> str:
    value = max_numeric_suffix(root, prefix) + 1
    while f"{prefix}__{value}" in parent:
        value += 1
    return f"{prefix}__{value}"


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


def rename_child_key(root: dict, parent: dict, old_key: str, new_key: str) -> None:
    items = []
    for key, value in parent.items():
        if key == old_key:
            items.append((new_key, value))
        else:
            items.append((key, value))
    parent.clear()
    for key, value in items:
        parent[key] = value


def find_direct_child_key(parent: dict, caption: str) -> str | None:
    for key, value in parent.items():
        if isinstance(value, dict) and value.get("caption") == caption:
            return key
    return None


def main() -> int:
    data, header, _ = decode_ftu(MAIN_FTU)
    stamp = time.strftime("%Y%m%d_%H%M%S")
    backup_dir = RELEASE / f"normalize_main_ftu_ide_keys_backup_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup_dir / "main.before.ftu")

    changed = False

    sys_key = find_direct_child_key(data, "sys_back")
    if sys_key is None:
        raise RuntimeError("sys_back missing; restore it before normalizing keys")
    if not re.fullmatch(r"button__\d+", sys_key):
        new_key = next_key(data, data, "button")
        rename_child_key(data, data, sys_key, new_key)
        print(f"renamed sys_back key {sys_key} -> {new_key}")
        changed = True
    else:
        print(f"sys_back key already numeric: {sys_key}")

    cycle_hits = find_caption(data, "CycleWindow")
    if not cycle_hits:
        raise RuntimeError("CycleWindow missing")
    cycle = cycle_hits[0][1]
    cancel_key = find_direct_child_key(cycle, "CycleCancelButton")
    if cancel_key is None:
        raise RuntimeError("CycleCancelButton missing; restore it before normalizing keys")
    if not re.fullmatch(r"button__\d+", cancel_key):
        new_key = next_key(data, cycle, "button")
        rename_child_key(data, cycle, cancel_key, new_key)
        print(f"renamed CycleCancelButton key {cancel_key} -> {new_key}")
        changed = True
    else:
        print(f"CycleCancelButton key already numeric: {cancel_key}")

    if changed:
        MAIN_FTU.write_bytes(encode_ftu(data, header))
        shutil.copy2(MAIN_FTU, backup_dir / "main.after.ftu")
    else:
        shutil.copy2(MAIN_FTU, backup_dir / "main.after.ftu")

    decoded, _, _ = decode_ftu(MAIN_FTU)
    for caption in ["sys_back", "CycleCancelButton"]:
        hits = find_caption(decoded, caption)
        if not hits:
            raise RuntimeError(f"{caption} missing after normalize")
        path, node = hits[0]
        print(f"OK {caption} path={path} id={node.get('id')} pos={node.get('position')}")
    print(f"backup_dir={backup_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
