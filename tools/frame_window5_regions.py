#!/usr/bin/env python3
"""Give the two Window5 child regions white framed backgrounds."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
REGION_PICTURES = {
    "Window5Region1Window": "debug_region_left_430x340.png",
    "Window5Region2Window": "debug_region_right_465x340.png",
}


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
    parent = find_caption(layout, "Window5")

    for caption, picture in REGION_PICTURES.items():
        region = find_caption(parent, caption)
        picture_path = ROOT / "resources" / picture
        if not picture_path.is_file():
            raise FileNotFoundError(picture_path)
        region["backgroundPic"] = picture

    UI_PATH.write_bytes(encode_ftu(layout, header))

    decoded, _, _ = decode_ftu(UI_PATH)
    decoded_parent = find_caption(decoded, "Window5")
    for caption, picture in REGION_PICTURES.items():
        if find_caption(decoded_parent, caption).get("backgroundPic") != picture:
            raise RuntimeError(f"failed to frame {caption}")

    print("framed Window5 child regions with white backgrounds")


if __name__ == "__main__":
    main()
