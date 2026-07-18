#!/usr/bin/env python3
"""Cover GroupNameEditText text with its icon while preserving edit interaction."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except LookupError:
                pass
    raise LookupError(caption)


def find_parent(node: object, child: dict) -> dict:
    if isinstance(node, dict):
        if any(value is child for value in node.values()):
            return node
        for value in node.values():
            try:
                return find_parent(value, child)
            except LookupError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_parent(value, child)
            except LookupError:
                pass
    raise LookupError("parent")


def main() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    edit = find_caption(layout, "GroupNameEditText")
    parent = find_parent(layout, edit)

    try:
        old_overlay = find_caption(parent, "GroupNameIconOverlay")
        old_key = next(key for key, value in parent.items() if value is old_overlay)
        del parent[old_key]
    except LookupError:
        pass

    parent["button__180"] = {
        "alignment": 37,
        "beepEnable": False,
        "caption": "GroupNameIconOverlay",
        "id": 20904,
        "picTab": {
            "pic0": "w2_set_rename_group_110x113_v3.png",
            "pic1": "w2_set_rename_group_110x113_v3.png",
        },
        "touchable": False,
        "position": dict(edit["position"]),
    }
    UI_PATH.write_bytes(encode_ftu(layout, header))

    verified, _, _ = decode_ftu(UI_PATH)
    overlay = find_caption(verified, "GroupNameIconOverlay")
    if overlay.get("position") != edit["position"] or overlay.get("touchable") is not False:
        raise RuntimeError("GroupNameIconOverlay verification failed")
    print(f"added non-touchable group-name overlay at {edit['position']}")


if __name__ == "__main__":
    main()
