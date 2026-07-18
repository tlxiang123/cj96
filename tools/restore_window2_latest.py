#!/usr/bin/env python3
"""Restore Window2 and its add-device editor from the latest UI snapshot."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
TARGET = ROOT / "ui" / "main.ftu"
BACKUP = ROOT / "backups" / "window8_compact_ring_legend_20260716_182953" / "ui" / "main.ftu"


def replace_caption(node: object, replacements: dict[str, dict]) -> None:
    if isinstance(node, dict):
        for key, value in list(node.items()):
            if isinstance(value, dict):
                caption = value.get("caption")
                if caption in replacements:
                    node[key] = replacements[caption]
                else:
                    replace_caption(value, replacements)
            elif isinstance(value, list):
                replace_caption(value, replacements)
    elif isinstance(node, list):
        for value in node:
            replace_caption(value, replacements)


def collect(node: object, names: set[str], output: dict[str, dict]) -> None:
    if isinstance(node, dict):
        caption = node.get("caption")
        if caption in names:
            output[caption] = node
        for value in node.values():
            collect(value, names, output)
    elif isinstance(node, list):
        for value in node:
            collect(value, names, output)


def main() -> None:
    current, header, _ = decode_ftu(TARGET)
    backup, _, _ = decode_ftu(BACKUP)
    replacement: dict[str, dict] = {}
    collect(backup, {"Window2", "w2set_window"}, replacement)
    if set(replacement) != {"Window2", "w2set_window"}:
        raise RuntimeError("latest Window2 snapshot is incomplete")
    replace_caption(current, replacement)
    TARGET.write_bytes(encode_ftu(current, header))
    print("restored Window2 and w2set_window from latest snapshot")


if __name__ == "__main__":
    main()
