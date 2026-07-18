#!/usr/bin/env python3
"""Replace the Window10 address editor with a fixed image-backed display."""

from __future__ import annotations

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


def main() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    window = find_caption(layout, "Window10")
    label = find_caption(window, "TextView1")
    address_view = None
    for caption in ("W2_AddressEditText", "W2_AddressTextView"):
        try:
            address_view = find_caption(window, caption)
            break
        except LookupError:
            pass

    label.clear()
    label.update({
        "alignment": 37,
        "backgroundPic": "w2_set_address_combined_020.png",
        "caption": "TextView1",
        "colorTab": {"color0": 0xFFFFFF},
        "id": 50018,
        "touchable": False,
        "position": {"height": 77, "left": 43, "top": 18, "width": 199},
    })
    if address_view is not None:
        address_key = next(key for key, value in window.items() if value is address_view)
        del window[address_key]

    name_label = find_caption(window, "TextView3")
    name_label.clear()
    name_label.update({
        "alignment": 37,
        "backgroundPic": "w2_set_device_name_label_77x77.png",
        "caption": "TextView3",
        "colorTab": {"color0": 0xFFFFFF},
        "id": 50019,
        "touchable": False,
        "position": {"height": 77, "left": 280, "top": 18, "width": 77},
    })

    change_list = find_caption(window, "ChangeIrr_ListView")
    change_list["position"]["top"] = 95

    UI_PATH.write_bytes(encode_ftu(layout, header))


if __name__ == "__main__":
    main()
