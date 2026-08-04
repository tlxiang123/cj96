#!/usr/bin/env python3
"""Add AM/PM text and a network time-sync failure dialog."""

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "showsysdate.ftu"
RESOURCE = ROOT / "resources" / "time_sync_failure_440x210.png"
BLUE = 0x168BFF
BLUE_DARK = 0x005BBB
PANEL = 0xF7FBFF
PRESSED = 0xEAF6FF
WHITE = 0xFFFFFF


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


def build_dialog() -> None:
    scale = 4
    image = Image.new("RGBA", (440 * scale, 210 * scale), (0, 0, 0, 0))
    ImageDraw.Draw(image).rounded_rectangle(
        (2 * scale, 2 * scale, 438 * scale, 208 * scale),
        radius=16 * scale,
        fill=(247, 251, 255, 255),
        outline=(22, 139, 255, 255),
        width=2 * scale,
    )
    image.resize((440, 210), Image.Resampling.LANCZOS).save(RESOURCE, optimize=True)


def main() -> None:
    build_dialog()
    data, header, _ = decode_ftu(UI_PATH)
    root = find_caption(data, "Window1")
    clock = find_caption(root, "DigitalClock1")
    clock["position"] = {"height": 44, "left": 110, "top": 362, "width": 180}

    root["textview__ampm"] = {
        "alignment": 36,
        "caption": "AmPmText",
        "colorTab": {"color0": BLUE_DARK},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 22,
        "id": 50060,
        "position": {"height": 44, "left": 294, "top": 362, "width": 72},
        "text": "AM",
        "touchable": False,
        "visible": False,
    }
    root["window__sync_failure"] = {
        "backgroundPic": RESOURCE.name,
        "beepEnable": True,
        "caption": "SyncFailureWindow",
        "id": 110020,
        "modal": True,
        "position": {"height": 210, "left": 292, "top": 195, "width": 440},
        "visible": False,
        "textview__title": {
            "alignment": 37,
            "caption": "SyncFailureTitle",
            "colorTab": {"color0": 0x111318},
            "family": "Alibaba-PuHuiTi-Regular",
            "fontSize": 26,
            "id": 50061,
            "position": {"height": 48, "left": 70, "top": 25, "width": 300},
            "text": "时间同步失败",
            "touchable": False,
        },
        "textview__message": {
            "alignment": 37,
            "caption": "SyncFailureMessage",
            "colorTab": {"color0": 0x526579},
            "family": "Alibaba-PuHuiTi-Regular",
            "fontSize": 20,
            "id": 50062,
            "position": {"height": 42, "left": 40, "top": 78, "width": 360},
            "text": "网络故障，同步失败",
            "touchable": False,
        },
        "button__ok": {
            "alignment": 37,
            "beepEnable": True,
            "bgColorTab": {"color0": BLUE, "color1": BLUE_DARK},
            "caption": "SyncFailureOkButton",
            "colorTab": {"color0": WHITE, "color1": WHITE},
            "family": "Alibaba-PuHuiTi-Regular",
            "fontSize": 22,
            "id": 20021,
            "position": {"height": 48, "left": 150, "top": 142, "width": 140},
            "text": "确定",
        },
    }
    UI_PATH.write_bytes(encode_ftu(data, header))
    print(f"updated {UI_PATH}")


if __name__ == "__main__":
    main()
