#!/usr/bin/env python3
"""Match Button1 background to the surrounding page without moving it."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "page1topset.ftu"


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
    page = find_caption(data, "TopSetPageWindow")
    button = find_caption(data, "Button1")
    original_position = dict(button["position"])
    original_icon_position = dict(button["iconPosition"])

    background = page.get("backgroundColor", 16777215)
    button["bgColorTab"] = {"color0": background, "color1": background}

    if button["position"] != original_position:
        raise RuntimeError("Button1 position changed unexpectedly")
    if button["iconPosition"] != original_icon_position:
        raise RuntimeError("Button1 icon position changed unexpectedly")

    UI_PATH.write_bytes(encode_ftu(data, header))
    print(f"Button1 background={background}, position={original_position}")


if __name__ == "__main__":
    main()
