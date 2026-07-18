#!/usr/bin/env python3
"""Restore Window3 as the startup page without changing its layout or colors."""

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
    window1 = find_control(data, "Window1")
    window3 = find_control(data, "Window3")
    window8 = find_control(data, "Window8")
    if window1 is None or window3 is None or window8 is None:
        raise RuntimeError("required startup windows are missing")

    window1["visible"] = False
    window8["visible"] = False
    window3.pop("visible", None)

    FTU_PATH.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(FTU_PATH)
    checks = {
        "Window1": find_control(decoded, "Window1").get("visible", True),
        "Window3": find_control(decoded, "Window3").get("visible", True),
        "Window8": find_control(decoded, "Window8").get("visible", True),
    }
    if checks != {"Window1": False, "Window3": True, "Window8": False}:
        raise RuntimeError(f"visibility verification failed: {checks}")
    print(checks)


if __name__ == "__main__":
    main()
