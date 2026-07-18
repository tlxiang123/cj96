#!/usr/bin/env python3
"""Make Button9 interactive and Button28 a non-interactive separator."""

from __future__ import annotations

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "main.ftu"


def find_control(node: object, caption: str) -> dict | None:
    if not isinstance(node, dict):
        return None
    if node.get("caption") == caption:
        return node
    for value in node.values():
        if isinstance(value, dict):
            found = find_control(value, caption)
            if found is not None:
                return found
    return None


def main() -> None:
    data, header, _ = decode_ftu(FTU_PATH)
    settings = find_control(data, "Button9")
    separator = find_control(data, "Button28")
    if settings is None or separator is None:
        raise RuntimeError("Button9 or Button28 is missing")
    if settings.get("id") != 20082 or separator.get("id") != 20009:
        raise RuntimeError("unexpected Button9/Button28 control IDs")

    settings["touchable"] = True
    separator["touchable"] = False

    FTU_PATH.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(FTU_PATH)
    verified_settings = find_control(decoded, "Button9")
    verified_separator = find_control(decoded, "Button28")
    if not verified_settings.get("touchable"):
        raise RuntimeError("Button9 is not touchable")
    if verified_separator.get("touchable", True):
        raise RuntimeError("Button28 is still touchable")
    print("Button9 touchable; Button28 non-interactive")


if __name__ == "__main__":
    main()
