#!/usr/bin/env python3
"""Apply the system-settings header asset to page1topset.ftu."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "page1topset.ftu"
ASSET_NAME = "topset_system_settings_header_158x100.png"


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
    title = find_caption(data, "TextView7")
    title["backgroundPic"] = ASSET_NAME
    title["text"] = ""
    title["position"] = {"height": 100, "left": 433, "top": 0, "width": 158}
    UI_PATH.write_bytes(encode_ftu(data, header))
    print(f"updated {UI_PATH}")


if __name__ == "__main__":
    main()
