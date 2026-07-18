#!/usr/bin/env python3
"""Center W2_NameEditText without changing its IDE-managed position or size."""

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
    control = find_caption(layout, "W2_NameEditText")
    position = dict(control.get("position", {}))
    control["alignment"] = 37
    UI_PATH.write_bytes(encode_ftu(layout, header))

    verified, _, _ = decode_ftu(UI_PATH)
    result = find_caption(verified, "W2_NameEditText")
    if result.get("alignment") != 37 or result.get("position") != position:
        raise RuntimeError("W2_NameEditText alignment verification failed")
    print(f"centered W2_NameEditText; preserved position {position}")


if __name__ == "__main__":
    main()
