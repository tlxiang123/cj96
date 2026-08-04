#!/usr/bin/env python3
"""Give W2TipWindow enough text area for wrapped valve address ranges."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from ftu_style import decode_ftu, encode_ftu  # noqa: E402


FTU_PATH = ROOT / "ui" / "main.ftu"


def find_control(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def main() -> None:
    data, header, _ = decode_ftu(FTU_PATH)
    window = find_control(data, "W2TipWindow")
    text = find_control(window, "W2TextView")
    text["alignment"] = 37
    text["fontSize"] = 24
    text["position"] = {"height": 190, "left": 25, "top": 15, "width": 510}
    FTU_PATH.write_bytes(encode_ftu(data, header))

    verified, _, _ = decode_ftu(FTU_PATH)
    result = find_control(verified, "W2TextView")
    if result.get("fontSize") != 24 or result.get("position") != text["position"]:
        raise RuntimeError("W2TextView layout verification failed")
    print("expanded W2TextView for wrapped valve address ranges")


if __name__ == "__main__":
    main()
