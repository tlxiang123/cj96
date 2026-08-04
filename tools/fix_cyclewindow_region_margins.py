#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, decode_ftu, encode_ftu


MAIN_FTU = UI_DIR / "main.ftu"


def find_caption(node: object, caption: str) -> dict | None:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            found = find_caption(value, caption)
            if found is not None:
                return found
    elif isinstance(node, list):
        for value in node:
            found = find_caption(value, caption)
            if found is not None:
                return found
    return None


def main() -> None:
    data, header, _ = decode_ftu(MAIN_FTU)
    cycle = find_caption(data, "CycleWindow")
    if cycle is None:
        raise RuntimeError("CycleWindow not found")

    left = find_caption(cycle, "CycleRegionLeftWindow")
    right = find_caption(cycle, "CycleRegionRightWindow")
    ok = find_caption(cycle, "CycleOKButton")
    cancel = find_caption(cycle, "CycleCancelButton")
    if left is None or right is None or ok is None or cancel is None:
        raise RuntimeError("CycleWindow region/button missing")

    ok_before = ok.get("picTab")
    cancel_before = cancel.get("picTab")

    left["position"] = {"left": 20, "top": 12, "width": 390, "height": 280}
    right["position"] = {"left": 467, "top": 12, "width": 520, "height": 280}

    if ok.get("picTab") != ok_before or cancel.get("picTab") != cancel_before:
        raise RuntimeError("button style changed unexpectedly")

    MAIN_FTU.write_bytes(encode_ftu(data, header))

    decoded, _, _ = decode_ftu(MAIN_FTU)
    decoded_cycle = find_caption(decoded, "CycleWindow")
    decoded_left = find_caption(decoded_cycle, "CycleRegionLeftWindow")
    decoded_right = find_caption(decoded_cycle, "CycleRegionRightWindow")
    print("CycleRegionLeftWindow", decoded_left["position"])
    print("CycleRegionRightWindow", decoded_right["position"])


if __name__ == "__main__":
    main()
