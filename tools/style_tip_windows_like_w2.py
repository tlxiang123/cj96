#!/usr/bin/env python3
"""Apply the current W2TipWindow frame style to selected settings popups."""

from __future__ import annotations

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_DIR = ROOT / "ui"

W2_FRAME = "w2_group_rename_dialog_560x220.png"
WINDOW_POS = {"height": 220, "left": 232, "top": 190, "width": 560}
TEXT_BLUE = 0x005BBB
TEXT_DARK = 0x111318
WHITE = 0xFFFFFF
BUTTON_BLUE = 0x168BFF
BUTTON_BLUE_PRESSED = 0x005BBB


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


def control_signature(node: object) -> list[tuple[str, int | None]]:
    result: list[tuple[str, int | None]] = []
    if isinstance(node, dict):
        if "caption" in node:
            result.append((str(node["caption"]), node.get("id")))
        for value in node.values():
            result.extend(control_signature(value))
    elif isinstance(node, list):
        for value in node:
            result.extend(control_signature(value))
    return result


def style_window(window: dict) -> None:
    window["position"] = dict(WINDOW_POS)
    window["backgroundPic"] = W2_FRAME
    window.pop("backgroundColor", None)


def style_text(
    control: dict,
    *,
    left: int,
    top: int,
    width: int,
    height: int,
    font_size: int = 24,
    color: int = TEXT_BLUE,
    alignment: int = 37,
    bold: bool = False,
) -> None:
    control["position"] = {
        "height": height,
        "left": left,
        "top": top,
        "width": width,
    }
    control["fontSize"] = font_size
    control["alignment"] = alignment
    control["colorTab"] = {"color0": color}
    control["family"] = "Alibaba-PuHuiTi-Regular"
    if bold:
        control["bold"] = True
    else:
        control.pop("bold", None)
    control.pop("backgroundColor", None)
    control.pop("bgColorTab", None)


def style_button(button: dict, *, left: int, top: int, width: int, height: int) -> None:
    button["position"] = {
        "height": height,
        "left": left,
        "top": top,
        "width": width,
    }
    button["fontSize"] = 22
    button["alignment"] = 37
    button["colorTab"] = {"color0": WHITE, "color1": WHITE}
    button["bgColorTab"] = {"color0": BUTTON_BLUE, "color1": BUTTON_BLUE_PRESSED}
    button["family"] = "Alibaba-PuHuiTi-Regular"


def style_ethernet() -> None:
    path = UI_DIR / "ethernetsetting.ftu"
    data, header, _ = decode_ftu(path)
    before = sorted(control_signature(data))

    save_tip_win = find_caption(data, "saveTipWin")
    save_tip = find_caption(save_tip_win, "saveTip")
    style_window(save_tip_win)
    style_text(
        save_tip,
        left=40,
        top=0,
        width=480,
        height=220,
        font_size=30,
        color=TEXT_BLUE,
        alignment=37,
        bold=True,
    )

    path.write_bytes(encode_ftu(data, header))
    verified, _, _ = decode_ftu(path)
    if sorted(control_signature(verified)) != before:
        raise RuntimeError("ethernetsetting.ftu control IDs/captions changed")
    if find_caption(verified, "saveTipWin").get("backgroundPic") != W2_FRAME:
        raise RuntimeError("saveTipWin style was not written")


def style_wifi() -> None:
    path = UI_DIR / "wifisetting.ftu"
    data, header, _ = decode_ftu(path)
    before = sorted(control_signature(data))

    disconnect = find_caption(data, "WindowDisconnect")
    password_error = find_caption(data, "WindowPasswordError")
    menu_more = find_caption(data, "WindowMenuMore")

    style_window(disconnect)
    style_text(find_caption(disconnect, "Textview15"), left=52, top=32, width=150, height=34, font_size=23, color=TEXT_DARK, alignment=38)
    style_text(find_caption(disconnect, "Textview17"), left=52, top=78, width=150, height=34, font_size=23, color=TEXT_DARK, alignment=38)
    style_text(find_caption(disconnect, "Textview19"), left=52, top=124, width=150, height=34, font_size=23, color=TEXT_DARK, alignment=38)
    style_text(find_caption(disconnect, "TextConnectSsid"), left=206, top=32, width=300, height=34, font_size=23, color=TEXT_BLUE, alignment=36, bold=True)
    style_text(find_caption(disconnect, "TextConnectSecType"), left=206, top=78, width=300, height=34, font_size=23, color=TEXT_BLUE, alignment=36, bold=True)
    style_text(find_caption(disconnect, "TextConnectState"), left=206, top=124, width=250, height=34, font_size=23, color=TEXT_BLUE, alignment=36, bold=True)
    style_button(find_caption(disconnect, "ButtonDisconnect"), left=390, top=160, width=130, height=42)

    style_window(password_error)
    style_text(
        find_caption(password_error, "Textview27"),
        left=40,
        top=0,
        width=480,
        height=220,
        font_size=30,
        color=TEXT_BLUE,
        alignment=37,
        bold=True,
    )

    style_window(menu_more)
    style_text(find_caption(menu_more, "Textview2"), left=70, top=56, width=150, height=40, font_size=24, color=TEXT_DARK, alignment=38)
    style_text(find_caption(menu_more, "Textview6"), left=70, top=120, width=150, height=40, font_size=24, color=TEXT_DARK, alignment=38)
    style_text(find_caption(menu_more, "TextIPAddr"), left=220, top=56, width=290, height=40, font_size=24, color=TEXT_BLUE, alignment=36, bold=True)
    style_text(find_caption(menu_more, "TextMacAddr"), left=220, top=120, width=290, height=40, font_size=24, color=TEXT_BLUE, alignment=36, bold=True)

    path.write_bytes(encode_ftu(data, header))
    verified, _, _ = decode_ftu(path)
    if sorted(control_signature(verified)) != before:
        raise RuntimeError("wifisetting.ftu control IDs/captions changed")
    for caption in ("WindowDisconnect", "WindowPasswordError", "WindowMenuMore"):
        if find_caption(verified, caption).get("backgroundPic") != W2_FRAME:
            raise RuntimeError(f"{caption} style was not written")


def main() -> None:
    if not (ROOT / "resources" / W2_FRAME).exists():
        raise FileNotFoundError(ROOT / "resources" / W2_FRAME)
    style_ethernet()
    style_wifi()
    print("styled saveTipWin, WindowDisconnect, WindowPasswordError, WindowMenuMore like W2TipWindow")


if __name__ == "__main__":
    main()
