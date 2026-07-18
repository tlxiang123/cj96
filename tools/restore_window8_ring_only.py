#!/usr/bin/env python3
"""Restore only the Window8 three-color ring dashboard."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
CURRENT = ROOT / "ui" / "main.ftu"
SOURCE = ROOT / "backups" / "window8_compact_ring_legend_20260716_182953" / "ui" / "main.ftu"


def find_control(node: object, caption: str) -> tuple[dict, str, dict]:
    if isinstance(node, dict):
        for key, value in node.items():
            if isinstance(value, dict) and value.get("caption") == caption:
                return node, key, value
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


current, header, _ = decode_ftu(CURRENT)
source, _, _ = decode_ftu(SOURCE)
current_parent, current_key, _ = find_control(current, "Window8")
_, _, source_window = find_control(source, "Window8")
current_parent[current_key] = source_window
CURRENT.write_bytes(encode_ftu(current, header))

verified, _, _ = decode_ftu(CURRENT)
_, _, window = find_control(verified, "Window8")
expected = {
    "Window8CompletedGroupText", "Window8RunningGroupText", "Window8WaitingGroupText",
    "Window8StatusLine1Text", "Window8StatusLine2Text", "Window8StatusLine3Text",
}
found = {value.get("caption") for value in window.values() if isinstance(value, dict)}
if not expected.issubset(found) or window.get("backgroundPic") != "window8_panel.png":
    raise RuntimeError("Window8 ring dashboard verification failed")
print("restored Window8 three-color ring dashboard only")
