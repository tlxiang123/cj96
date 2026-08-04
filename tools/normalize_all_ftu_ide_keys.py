#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Normalize all manually named FTU control keys under ui/.

FlyThingsIDE-created controls use keys like button__184. Manually inserted
keys like button__global_screenshot can remain in the encoded file but not
render reliably in the IDE canvas. This script renames only the JSON object
keys; it preserves caption, id, position, images, text, and all other fields.
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
UI = ROOT / "ui"
RELEASE = ROOT / "Release"

CONTROL_PREFIXES = {
    "button",
    "window",
    "textview",
    "edittext",
    "digitalclock",
    "checkbox",
    "radiobuttons",
    "seekbar",
    "imageview",
    "list",
    "item",
    "subitem",
    "progressbar",
    "slidewindow",
    "scrollwindow",
    "pagewindow",
    "video",
    "camera",
    "qrcode",
    "waveform",
    "diagram",
    "circlebar",
    "pointer",
    "painter",
}


def split_control_key(key: str) -> tuple[str, str] | None:
    if "__" not in key:
        return None
    prefix, suffix = key.split("__", 1)
    prefix = prefix.lower()
    if prefix not in CONTROL_PREFIXES:
        return None
    return prefix, suffix


def is_numeric_control_key(key: str) -> bool:
    parsed = split_control_key(key)
    return bool(parsed and parsed[1].isdigit())


def is_non_numeric_control_key(key: str) -> bool:
    parsed = split_control_key(key)
    return bool(parsed and not parsed[1].isdigit())


def max_numeric_suffix(node: object, prefix: str) -> int:
    max_value = 0
    if isinstance(node, dict):
        for key, value in node.items():
            parsed = split_control_key(key)
            if parsed and parsed[0] == prefix and parsed[1].isdigit():
                max_value = max(max_value, int(parsed[1]))
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


def normalize_node(root: dict, node: object, changes: list[tuple[str, str, str, str]]) -> None:
    if isinstance(node, dict):
        original_items = list(node.items())
        node.clear()
        for key, value in original_items:
            new_key = key
            parsed = split_control_key(key)
            if parsed and not parsed[1].isdigit() and isinstance(value, dict):
                new_key = next_key(root, node, parsed[0])
                caption = str(value.get("caption") or "")
                changes.append((key, new_key, caption, str(value.get("id") or "")))
            node[new_key] = value
            normalize_node(root, value, changes)
    elif isinstance(node, list):
        for value in node:
            normalize_node(root, value, changes)


def scan_issues(data: dict) -> list[tuple[str, str, str]]:
    issues: list[tuple[str, str, str]] = []

    def walk(node: object, path: str = "") -> None:
        if isinstance(node, dict):
            for key, value in node.items():
                child_path = f"{path}/{key}"
                if is_non_numeric_control_key(key) and isinstance(value, dict):
                    issues.append((child_path, str(value.get("caption") or ""), str(value.get("id") or "")))
                walk(value, child_path)
        elif isinstance(node, list):
            for i, value in enumerate(node):
                walk(value, f"{path}[{i}]")

    walk(data)
    return issues


def main() -> int:
    stamp = time.strftime("%Y%m%d_%H%M%S")
    backup_dir = RELEASE / f"normalize_all_ftu_ide_keys_backup_{stamp}"
    backup_dir.mkdir(parents=True, exist_ok=False)

    total_changes = 0
    for ftu in sorted(UI.glob("*.ftu")):
        data, header, _ = decode_ftu(ftu)
        before_issues = scan_issues(data)
        if not before_issues:
            print(f"OK {ftu.name}: no non-numeric control keys")
            continue

        shutil.copy2(ftu, backup_dir / f"{ftu.name}.before")
        changes: list[tuple[str, str, str, str]] = []
        normalize_node(data, data, changes)
        ftu.write_bytes(encode_ftu(data, header))
        shutil.copy2(ftu, backup_dir / f"{ftu.name}.after")

        decoded, _, _ = decode_ftu(ftu)
        after_issues = scan_issues(decoded)
        if after_issues:
            raise RuntimeError(f"{ftu} still has non-numeric keys: {after_issues[:5]}")

        total_changes += len(changes)
        print(f"UPDATED {ftu.name}: {len(changes)} keys")
        for old, new, caption, control_id in changes:
            print(f"  {old} -> {new} caption={caption} id={control_id}")

    print(f"total_changes={total_changes}")
    print(f"backup_dir={backup_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
