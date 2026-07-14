#!/usr/bin/env python3
"""Print FTU controls with the properties needed for visual restyling."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

from ftu_style import UI_DIR, decode_ftu


def walk(node: object, path: str = "") -> None:
    if not isinstance(node, dict):
        return
    for key, value in node.items():
        if not isinstance(value, dict):
            continue
        current = f"{path}/{key}"
        if "caption" in value or key.split("__", 1)[0] in {
            "screen",
            "window",
            "pagewindow",
            "listview",
            "item",
        }:
            props = {
                name: value.get(name)
                for name in (
                    "caption",
                    "id",
                    "position",
                    "text",
                    "backgroundColor",
                    "textColor",
                    "backgroundPic",
                    "icon",
                    "visible",
                )
                if name in value
            }
            print(f"{current}: {props}")
        walk(value, current)


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    parser = argparse.ArgumentParser()
    parser.add_argument("name", help="FTU filename or stem")
    args = parser.parse_args()
    name = args.name if args.name.endswith(".ftu") else f"{args.name}.ftu"
    data, _, _ = decode_ftu(Path(UI_DIR) / name)
    walk(data)


if __name__ == "__main__":
    main()
