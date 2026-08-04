#!/usr/bin/env python3
"""Restyle wifisetting with the page1topset visual frame."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "wifisetting.ftu"
RESOURCE_DIR = ROOT / "resources"

WHITE = 0xFFFFFF
TEXT = 0x111318
MUTED = 0x526579
BLUE = 0x168BFF
BLUE_PRESSED = 0x005BBB
ROW_SELECTED = 0xEAF6FF


def find_control(node: object, caption: str) -> dict | None:
    if not isinstance(node, dict):
        return None
    if node.get("caption") == caption:
        return node
    for value in node.values():
        if isinstance(value, dict):
            found = find_control(value, caption)
            if found is not None:
                return found
    return None


def rounded_frame(path: Path, size: tuple[int, int], radius: int = 14) -> None:
    scale = 4
    width, height = size
    image = Image.new("RGBA", (width * scale, height * scale), (255, 255, 255, 255))
    draw = ImageDraw.Draw(image)
    inset = 2 * scale
    draw.rounded_rectangle(
        (inset, inset, image.width - inset - 1, image.height - inset - 1),
        radius=radius * scale,
        outline=(73, 174, 238, 255),
        width=2 * scale,
    )
    image.resize(size, Image.Resampling.LANCZOS).save(path, optimize=True)


def style_text(control: dict, size: int = 20, color: int = TEXT) -> None:
    control["fontSize"] = size
    control["colorTab"] = {"color0": color, "color1": color, "color2": BLUE}
    control["family"] = "Alibaba-PuHuiTi-Regular"


def style_dialog(dialog: dict, frame_name: str) -> None:
    dialog["backgroundColor"] = WHITE
    dialog["backgroundPic"] = frame_name
    dialog["position"] = {"height": 250, "left": 292, "top": 175, "width": 440}
    for value in dialog.values():
        if not isinstance(value, dict):
            continue
        caption = value.get("caption", "")
        if caption.startswith("Text"):
            style_text(value)


def main() -> None:
    dialog_frame_name = "wifi_dialog_frame_440x250.png"
    rounded_frame(RESOURCE_DIR / dialog_frame_name, (440, 250))

    data, header, _ = decode_ftu(FTU_PATH)
    controls = {
        caption: find_control(data, caption)
        for caption in (
            "Window1", "Window2", "ListViewWifiInfo", "ButtonOnOff", "sys_back",
            "Textview1", "WindowSet", "WindowDisconnect", "WindowPasswordError",
            "WindowMenuMore", "ButtonConnect", "ButtonDisconnect", "ButtonShowPwd",
            "EdittextPwd",
        )
    }
    missing = [name for name, control in controls.items() if control is None]
    if missing:
        raise RuntimeError(f"missing Wi-Fi controls: {', '.join(missing)}")

    page = controls["Window1"]
    header_window = controls["Window2"]
    wifi_list = controls["ListViewWifiInfo"]
    on_off = controls["ButtonOnOff"]
    back = controls["sys_back"]
    title = controls["Textview1"]

    # Remove controls from their old parents before rebuilding the page hierarchy.
    for container in (page, header_window):
        for key in list(container):
            if isinstance(container[key], dict) and container[key] in (wifi_list, on_off, back, title):
                del container[key]

    back["picTab"] = {"pic0": "topset_back_36.png"}
    back["position"] = {"height": 70, "left": 0, "top": 0, "width": 87}
    back["iconPosition"] = {"height": 36, "left": 54, "top": 18, "width": 36}
    title["position"] = {"height": 70, "left": 358, "top": 15, "width": 308}
    title["fontSize"] = 34
    title["bold"] = True
    title["colorTab"] = {"color0": TEXT}
    on_off["position"] = {"height": 50, "left": 902, "top": 25, "width": 96}
    on_off["iconPosition"] = {"height": 43, "left": 45, "top": 3, "width": 43}

    header_window["backgroundPic"] = "nettopwind.png"
    header_window.pop("backgroundColor", None)
    header_window["position"] = {"height": 100, "left": 0, "top": 0, "width": 1024}
    header_window["button__1"] = back
    header_window["textview__2"] = title
    header_window["button__3"] = on_off

    wifi_list["position"] = {"height": 320, "left": 30, "top": 20, "width": 907}
    wifi_list["rows"] = 4
    item = wifi_list["item"]
    item["position"] = {"height": 80, "left": 0, "top": 0, "width": 907}
    item["backgroundColor"] = WHITE
    item["bgColorTab"] = {"color0": WHITE, "color1": ROW_SELECTED}
    item["colorTab"] = {"color0": TEXT}
    for subitem in item.get("subItem", []):
        subitem["backgroundColor"] = WHITE
        caption = subitem.get("caption")
        if caption == "LISTSUBITEM_LEVEL":
            subitem["position"] = {"height": 24, "left": 840, "top": 28, "width": 26}
        elif caption == "LISTSUBITEM_NAME":
            subitem["position"] = {"height": 40, "left": 34, "top": 5, "width": 750}
            style_text(subitem, 21)
        elif caption == "LISTSUBITEM_SUB":
            subitem["position"] = {"height": 28, "left": 34, "top": 44, "width": 750}
            style_text(subitem, 14, MUTED)
        elif caption == "SubItem1":
            subitem["position"] = {"height": 1, "left": 24, "top": 78, "width": 859}
            subitem["backgroundColor"] = 0xD8EAF6

    inner_frame = {
        "backgroundPic": "topset_inner_frame_967x360.png",
        "beepEnable": False,
        "caption": "WifiContentFrameWindow",
        "id": 110003,
        "position": {"height": 360, "left": 20, "top": 20, "width": 967},
        "window__1": wifi_list,
    }
    content = {
        "backgroundColor": WHITE,
        "beepEnable": True,
        "caption": "WifiContentWindow",
        "id": 110002,
        "position": {"height": 400, "left": 8, "top": 93, "width": 1007},
        "window__1": inner_frame,
    }
    page.clear()
    page.update({
        "backgroundColor": WHITE,
        "beepEnable": True,
        "caption": "Window1",
        "id": 100004,
        "position": {"height": 600, "left": 0, "top": 0, "width": 1024},
        "window__1": header_window,
        "window__2": content,
    })

    for name in ("WindowSet", "WindowDisconnect", "WindowMenuMore"):
        style_dialog(controls[name], dialog_frame_name)
    controls["WindowMenuMore"]["position"] = {"height": 170, "left": 292, "top": 190, "width": 440}

    for name in ("ButtonConnect", "ButtonDisconnect"):
        button = controls[name]
        button["position"] = {"height": 46, "left": 275, "top": 184, "width": 140}
        button["bgColorTab"] = {"color0": BLUE, "color1": BLUE_PRESSED}
        button["colorTab"] = {"color0": WHITE, "color1": WHITE}
        button["fontSize"] = 20
    controls["EdittextPwd"]["position"] = {"height": 38, "left": 147, "top": 125, "width": 260}
    controls["EdittextPwd"]["bgColorTab"] = {"color0": 0xF4F9FC}
    controls["ButtonShowPwd"]["position"] = {"height": 32, "left": 35, "top": 190, "width": 190}

    error = controls["WindowPasswordError"]
    error["position"] = {"height": 56, "left": 382, "top": 430, "width": 260}
    error_text = find_control(error, "Textview27")
    error_text["position"] = {"height": 56, "left": 0, "top": 0, "width": 260}
    error_text["backgroundColor"] = 0xFFF1F1
    error_text["colorTab"] = {"color0": 0xB42318}

    data["backgroundColor"] = WHITE
    data["window__1"] = page
    for key in list(data):
        if key.startswith("window__") and key != "window__1":
            del data[key]
    data["window__2"] = controls["WindowSet"]
    data["window__3"] = controls["WindowDisconnect"]
    data["window__4"] = controls["WindowPasswordError"]
    data["window__5"] = controls["WindowMenuMore"]

    FTU_PATH.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(FTU_PATH)
    for caption in controls:
        if find_control(decoded, caption) is None:
            raise RuntimeError(f"round-trip lost control: {caption}")
    print("restyled wifisetting with page1topset header and framed content")


if __name__ == "__main__":
    main()
