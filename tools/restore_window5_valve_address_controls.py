#!/usr/bin/env python3
"""Restore the editable valve-address controls on Window5."""

from __future__ import annotations

import shutil
from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
RESOURCE_DIR = ROOT / "resources"
UI_DIR = ROOT / "ui"


RESTORED_CONTROLS = {
    "textview__valve_addr_label": {
        "caption": "ValveAddressLabelText",
        "id": 50090,
        "position": {"left": 168, "top": 104, "width": 92, "height": 32},
        "text": "阀地址",
        "fontSize": 22,
        "colorTab": {"color0": 23483},
    },
    "button__valve_prev": {
        "caption": "ValveAddressPrevButton",
        "id": 20090,
        "position": {"left": 62, "top": 126, "width": 76, "height": 58},
        "text": "‹",
        "fontSize": 42,
        "colorTab": {"color0": 23483, "color1": 23483, "color2": 23483},
    },
    "edittext__valve_address": {
        "caption": "ValveAddressEditText",
        "id": 51040,
        "position": {"left": 196, "top": 112, "width": 99, "height": 71},
        "text": "20",
        "backgroundPic": "debug_edit_inner_99x71.png",
        "fontSize": 30,
        "alignment": 37,
        "colorTab": {"color0": 23483, "color1": 23483, "color2": 23483},
        "hintTextColor": 23483,
    },
    "button__valve_next": {
        "caption": "ValveAddressNextButton",
        "id": 20091,
        "position": {"left": 263, "top": 126, "width": 76, "height": 58},
        "text": "›",
        "fontSize": 42,
        "colorTab": {"color0": 23483, "color1": 23483, "color2": 23483},
    },
}


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def remove_existing_ids(node: object, ids: set[int]) -> None:
    if isinstance(node, dict):
        for key in list(node.keys()):
            value = node[key]
            if isinstance(value, dict) and value.get("id") in ids:
                del node[key]
            else:
                remove_existing_ids(value, ids)
    elif isinstance(node, list):
        node[:] = [
            value
            for value in node
            if not (isinstance(value, dict) and value.get("id") in ids)
        ]
        for value in node:
            remove_existing_ids(value, ids)


def find_id(node: object, control_id: int) -> dict:
    if isinstance(node, dict):
        if node.get("id") == control_id:
            return node
        for value in node.values():
            try:
                return find_id(value, control_id)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_id(value, control_id)
            except KeyError:
                pass
    raise KeyError(control_id)


def sync_required_assets() -> None:
    for name in ("debug_outline_76x58.png", "debug_edit_inner_99x71.png"):
        source = RESOURCE_DIR / name
        target = UI_DIR / name
        if source.exists() and not target.exists():
            shutil.copy2(source, target)


def main() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    window5 = find_caption(layout, "Window5")

    ids = {control["id"] for control in RESTORED_CONTROLS.values()}
    remove_existing_ids(window5, ids)
    window5.update(RESTORED_CONTROLS)

    UI_PATH.write_bytes(encode_ftu(layout, header))
    sync_required_assets()

    verified, _, _ = decode_ftu(UI_PATH)
    for control in RESTORED_CONTROLS.values():
        restored = find_id(verified, control["id"])
        if restored.get("caption") != control["caption"]:
            raise RuntimeError(f"failed to restore {control['caption']}")

    print("restored Window5 valve address label, edit box, and step buttons")


if __name__ == "__main__":
    main()
