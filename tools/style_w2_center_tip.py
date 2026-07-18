#!/usr/bin/env python3
"""Style W2TipWindow as a centered, buttonless dismiss-on-touch notice."""

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
UI_PATH = ROOT / "ui" / "main.ftu"
OUTPUT = RESOURCE_DIR / "w2_tip_center_560x140.png"


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


def build_image() -> None:
    scale = 4
    width, height = 560, 140
    image = Image.new("RGB", (width * scale, height * scale), (250, 254, 255))
    pixels = image.load()
    for y in range(height * scale):
        ratio = y / max(1, height * scale - 1)
        color = (round(254 - 8 * ratio), round(255 - 2 * ratio), 255)
        for x in range(width * scale):
            pixels[x, y] = color

    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (3 * scale, 3 * scale, (width - 4) * scale, (height - 4) * scale),
        radius=16 * scale,
        outline=(14, 151, 232),
        width=3 * scale,
    )
    image.resize((width, height), Image.Resampling.LANCZOS).save(OUTPUT, optimize=True)


def bind_image() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    tip_window = find_caption(layout, "W2TipWindow")
    tip_text = find_caption(tip_window, "W2TextView")
    w2_set_window = find_caption(layout, "w2set_window")

    tip_window["backgroundPic"] = OUTPUT.name
    tip_window["position"] = {"height": 140, "left": 220, "top": 155, "width": 560}

    tip_text.pop("bgColorTab", None)
    tip_text["alignment"] = 37
    tip_text["bold"] = True
    tip_text["colorTab"] = {"color0": 23483}
    tip_text["fontSize"] = 30
    tip_text["touchable"] = False
    tip_text["position"] = {"height": 140, "left": 0, "top": 0, "width": 560}

    try:
        action_tip = find_caption(w2_set_window, "W2ActionTipWindow")
        action_tip_key = next(key for key, value in w2_set_window.items() if value is action_tip)
        del w2_set_window[action_tip_key]
    except LookupError:
        pass
    w2_set_window["window__213"] = {
        "backgroundPic": OUTPUT.name,
        "beepEnable": True,
        "caption": "W2ActionTipWindow",
        "id": 110019,
        "visible": False,
        "position": {"height": 140, "left": 224, "top": 140, "width": 560},
        "textview__214": {
            "alignment": 37,
            "bold": True,
            "caption": "W2ActionTipTextView",
            "colorTab": {"color0": 23483},
            "fontSize": 30,
            "id": 50064,
            "text": "",
            "touchable": False,
            "position": {"height": 140, "left": 0, "top": 0, "width": 560},
        },
    }
    UI_PATH.write_bytes(encode_ftu(layout, header))

    verified, _, _ = decode_ftu(UI_PATH)
    result = find_caption(verified, "W2TipWindow")
    if result.get("position") != tip_window["position"] or result.get("backgroundPic") != OUTPUT.name:
        raise RuntimeError("W2TipWindow style verification failed")
    verified_action = find_caption(verified, "W2ActionTipWindow")
    if verified_action.get("position") != {"height": 140, "left": 224, "top": 140, "width": 560}:
        raise RuntimeError("W2ActionTipWindow style verification failed")
    print("styled centered 560x140 W2 tip windows")


def main() -> None:
    build_image()
    bind_image()


if __name__ == "__main__":
    main()
