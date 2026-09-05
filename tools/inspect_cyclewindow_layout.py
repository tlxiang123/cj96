#!/usr/bin/env python3
"""Print the CycleWindow controls needed for a focused layout change."""

from __future__ import annotations

import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ftu_style import decode_ftu


TARGETS = {
    "CycleWindow",
    "CycleCountEditText",
    "IntervalEditText",
    "Cycle1HourStartEditText",
    "Cycle1MinStartEditText",
    "Cycle1HourEndEditText",
    "Cycle1MinEndEditText",
    "Cycle2HourStartEditText",
    "Cycle2MinStartEditText",
    "Cycle2HourEndEditText",
    "Cycle2MinEndEditText",
    "Button23",
    "Button25",
    "Button28",
    "Button29",
    "Button30",
    "Button31",
    "Button32",
    "Button33",
    "Button34",
    "Button35",
    "Button36",
    "Button37",
    "Button38",
    "Button39",
    "CycleCancelButton",
    "CycleOKButton",
}


def collect(node: object, output: list[dict]) -> None:
    if isinstance(node, dict):
        if node.get("caption") in TARGETS:
            output.append(
                {
                    key: node.get(key)
                    for key in (
                        "caption",
                        "id",
                        "position",
                        "text",
                        "family",
                        "fontSize",
                        "alignment",
                        "colorTab",
                        "imeTextType",
                        "inputType",
                        "textType",
                        "backgroundPic",
                        "picTab",
                    )
                    if key in node
                }
            )
        for child in node.values():
            collect(child, output)
    elif isinstance(node, list):
        for child in node:
            collect(child, output)


def main() -> None:
    root = Path(__file__).resolve().parents[1]
    data, _, _ = decode_ftu(root / "ui" / "main.ftu")
    controls: list[dict] = []
    collect(data, controls)
    print(json.dumps(controls, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
