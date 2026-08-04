#!/usr/bin/env python3
"""Replace the system-time summary text with a live DigitalClock control."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "showsysdate.ftu"


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
    root = find_caption(data, "Window1")

    text_time = None
    for key, value in list(root.items()):
        if isinstance(value, dict) and value.get("caption") == "TextTime":
            text_time = value
            del root[key]
            break
    if text_time is None:
        raise RuntimeError("TextTime was not found at the Window1 level")

    digital_clock = {
        "alignment": text_time.get("alignment", 36),
        "caption": "DigitalClock1",
        "colorTab": text_time.get("colorTab", {"color0": 0x005BBB}),
        "family": text_time.get("family", "Alibaba-PuHuiTi-Regular"),
        "fontSize": text_time.get("fontSize", 25),
        "format": "HH:MM:SS",
        "id": 93001,
        "position": dict(text_time["position"]),
        "touchable": False,
    }
    root["digitalclock__system_time"] = digital_clock
    UI_PATH.write_bytes(encode_ftu(data, header))
    print(f"DigitalClock1 position={digital_clock['position']}")


if __name__ == "__main__":
    main()
