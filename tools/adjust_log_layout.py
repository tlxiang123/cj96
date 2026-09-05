#!/usr/bin/env python3
"""Keep the valve-operation log columns wide enough for the full date."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_FILE = ROOT / "ui" / "main.ftu"


def main() -> None:
    data, header, _ = decode_ftu(UI_FILE)
    log_window = next(
        value for value in data.values()
        if isinstance(value, dict) and value.get("caption") == "LogWindow"
    )
    changed = False
    column_layout = {
        "LogLine": (30, 250),
        "LogWeek": (280, 72),
        "LogMode": (360, 112),
        "LogAction": (482, 64),
        "LogDetail": (552, 348),
    }
    for index in range(1, 7):
        top = 102 + (index - 1) * 48
        for prefix, (left, width) in column_layout.items():
            control = next(
                value for value in log_window.values()
                if isinstance(value, dict)
                and value.get("caption") == f"{prefix}{index}"
            )
            wanted_position = {
                "height": 36,
                "left": left,
                "top": top,
                "width": width,
            }
            if control.get("position") != wanted_position:
                control["position"] = wanted_position
                changed = True

    if changed:
        UI_FILE.write_bytes(encode_ftu(data, header))
    print("log columns updated" if changed else "log columns already current")


if __name__ == "__main__":
    main()
