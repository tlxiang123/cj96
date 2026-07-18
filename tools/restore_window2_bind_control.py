#!/usr/bin/env python3
"""Restore Window2's group-name editor while keeping the w2set region editor."""

from copy import deepcopy
from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "main.ftu"


def find(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def pop_child(parent: dict, caption: str) -> tuple[str, dict]:
    for key, value in list(parent.items()):
        if isinstance(value, dict) and value.get("caption") == caption:
            del parent[key]
            return key, value
    raise KeyError(caption)


data, header, _ = decode_ftu(FTU_PATH)
bind_window = find(data, "GroupBindValueWindow")
region3 = find(data, "W2SetRegion3Window")

# The prior layout moved the only editor into region3. Put the original
# editor back in GroupBindValueWindow with its original ID and coordinates.
_, region_editor = pop_child(region3, "GroupNameEditText")
original_editor = deepcopy(region_editor)
original_editor["id"] = 51004
original_editor["position"] = {"height": 41, "left": 236, "top": 73, "width": 144}
bind_window["edittext__193"] = original_editor

# Keep a separate editor in the requested region so Window2 remains intact.
region_editor["id"] = 51039
region_editor["position"] = {"height": 41, "left": 154, "top": 170, "width": 144}
region3["edittext__w2_group_name"] = region_editor

FTU_PATH.write_bytes(encode_ftu(data, header))
print("restored Window2 group-name editor and retained w2set region editor")
