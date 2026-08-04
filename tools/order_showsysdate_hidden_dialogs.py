#!/usr/bin/env python3
"""Place system-time dialogs above the main-page controls at runtime."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "showsysdate.ftu"
DIALOGS = {"DatePickerWindow", "TimePickerWindow", "SyncFailureWindow"}


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
    metadata = []
    dialogs = []
    page_controls = []

    for key, value in root.items():
        if not (isinstance(value, dict) and value.get("caption")):
            metadata.append((key, value))
        elif value.get("caption") in DIALOGS:
            dialogs.append((key, value))
        else:
            page_controls.append((key, value))

    if len(dialogs) != len(DIALOGS):
        raise RuntimeError(f"expected {len(DIALOGS)} dialogs, found {len(dialogs)}")

    root.clear()
    root.update(metadata)
    root.update(page_controls)
    root.update(dialogs)
    UI_PATH.write_bytes(encode_ftu(data, header))
    captions = [
        value.get("caption")
        for value in root.values()
        if isinstance(value, dict) and value.get("caption")
    ]
    print(captions[-len(DIALOGS):])


if __name__ == "__main__":
    main()
