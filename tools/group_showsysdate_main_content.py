#!/usr/bin/env python3
"""Group the main time-page controls so dialogs can hide them at runtime."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "showsysdate.ftu"
KEEP_AT_ROOT = {
    "DatePickerWindow", "TimePickerWindow", "SyncFailureWindow",
    "sys_back", "TextView7",
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


def main() -> None:
    data, header, _ = decode_ftu(UI_PATH)
    root = find_caption(data, "Window1")
    main_content = {
        "beepEnable": True,
        "caption": "MainContentWindow",
        "id": 110030,
        "position": {"height": 600, "left": 0, "top": 0, "width": 1024},
    }

    moved = []
    for key, value in list(root.items()):
        if not (isinstance(value, dict) and value.get("caption")):
            continue
        if value.get("caption") in KEEP_AT_ROOT:
            continue
        moved.append((key, value))
        del root[key]

    for key, value in moved:
        main_content[key] = value
    root["window__main_content"] = main_content
    UI_PATH.write_bytes(encode_ftu(data, header))
    print(f"grouped {len(moved)} controls into MainContentWindow")


if __name__ == "__main__":
    main()
