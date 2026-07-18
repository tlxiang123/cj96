#!/usr/bin/env python3
"""Reparent Window4 controls into three rounded column windows."""

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
UI_PATH = ROOT / "ui" / "main.ftu"

PANELS = {
    "window4_region1_319x384.png": (319, 384),
    "window4_region2_319x384.png": (319, 384),
    "window4_region3_321x384.png": (321, 384),
}


def build_cycle_toggle_assets() -> None:
    for state in ("off", "on"):
        source = Image.open(
            RESOURCES / f"window3_cycle_toggle_{state}_65.png"
        ).convert("RGBA").crop((0, 17, 65, 48))
        image = Image.new("RGBA", (80, 43), (0, 0, 0, 0))
        image.paste(source, (7, 6), source)
        image.save(
            RESOURCES / f"window4_cycle_toggle_{state}_80x43.png",
            format="PNG",
            optimize=True,
        )


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


def pop_caption(node: object, caption: str) -> dict:
    if not isinstance(node, dict):
        raise LookupError(caption)
    for key, value in list(node.items()):
        if isinstance(value, dict) and value.get("caption") == caption:
            del node[key]
            return value
        if isinstance(value, dict):
            try:
                return pop_caption(value, caption)
            except LookupError:
                pass
    raise LookupError(caption)


def build_panel(path: Path, width: int, height: int) -> None:
    scale = 4
    image = Image.new("RGBA", (width * scale, height * scale), (0, 0, 0, 0))
    gradient = Image.new("RGBA", image.size)
    pixels = gradient.load()
    for y in range(height * scale):
        ratio = y / max(1, height * scale - 1)
        color = (round(250 - 10 * ratio), round(253 - 5 * ratio), 255, 255)
        for x in range(width * scale):
            pixels[x, y] = color
    mask = Image.new("L", image.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (2 * scale, 2 * scale, (width - 3) * scale, (height - 3) * scale),
        radius=10 * scale,
        fill=255,
    )
    image.paste(gradient, (0, 0), mask)
    ImageDraw.Draw(image).rounded_rectangle(
        (2 * scale, 2 * scale, (width - 3) * scale, (height - 3) * scale),
        radius=10 * scale,
        outline=(14, 151, 232, 255),
        width=2 * scale,
    )
    image.resize((width, height), Image.Resampling.LANCZOS).save(path, optimize=True)


def set_position(control: dict, left: int, top: int, width: int, height: int) -> None:
    control["position"] = {"height": height, "left": left, "top": top, "width": width}


def style_title(control: dict, width: int) -> None:
    set_position(control, 10, 8, width, 41)
    control["visible"] = True
    control["fontSize"] = 26
    control["bold"] = True
    control["touchable"] = False
    control["bgColorTab"] = {"color0": 15004927, "color1": 15004927}
    control["colorTab"] = {"color0": 23483, "color1": 23483}


def set_subitem_columns(item: dict, item_width: int, item_height: int) -> None:
    columns = (
        (0, 72),
        (76, item_width - 160),
        (item_width - 80, 80),
    )
    for subitem, (left, width) in zip(item.get("subItem", []), columns):
        set_position(subitem, left, 0, width, item_height)
        subitem["fontSize"] = 20
        subitem["colorTab"] = {"color0": 23483}
        subitem["bgColorTab"] = {"color0": 16186367}
        if subitem.get("picTab"):
            subitem["picTab"] = {
                "pic0": "window4_cycle_toggle_off_80x43.png",
                "pic2": "window4_cycle_toggle_on_80x43.png",
            }


def style_tip_list(control: dict, width: int) -> None:
    set_position(control, 10, 52, width, 38)
    control["rows"] = 1
    control["cols"] = 1
    control["rowSpacing"] = 0
    control["colSpacing"] = 0
    item = control["item"]
    set_position(item, 0, 0, width, 38)
    item["backgroundColor"] = 15004927
    item["bgColorTab"] = {"color0": 15004927, "color1": 15004927}
    set_subitem_columns(item, width, 38)


def style_value_list(control: dict, width: int) -> None:
    set_position(control, 10, 94, width, 278)
    control["rows"] = 6
    control["cols"] = 1
    control["rowSpacing"] = 3
    control["colSpacing"] = 0
    item = control["item"]
    set_position(item, 0, 0, width, 43)
    item["backgroundColor"] = 16186367
    item["bgColorTab"] = {
        "color0": 16186367,
        "color1": 14479087,
        "color2": 14479087,
    }
    set_subitem_columns(item, width, 43)


def main() -> None:
    for name, (width, height) in PANELS.items():
        build_panel(RESOURCES / name, width, height)
    build_cycle_toggle_assets()

    layout, header, _ = decode_ftu(UI_PATH)
    root = find_caption(layout, "Window4")
    groups = (
        ("Button20", "DeviceTestTipsListView", "DeviceTestValueListView"),
        ("Button21", "GroupTestTipsListView", "GroupTestValueListView"),
        ("Button22", "SenserTestTipsListView", "SenserTestValueListView"),
    )
    controls = {
        caption: pop_caption(root, caption)
        for group in groups
        for caption in group
    }
    for key, value in list(root.items()):
        if isinstance(value, dict) and value.get("caption", "").startswith("Window4Region"):
            del root[key]

    root["backgroundColor"] = 13624314
    root["visible"] = False
    root["position"] = {"height": 400, "left": 8, "top": 93, "width": 1007}

    specs = (
        (280, 110048, "Window4Region1Window", "window4_region1_319x384.png", 12, 319, groups[0]),
        (281, 110049, "Window4Region2Window", "window4_region2_319x384.png", 343, 319, groups[1]),
        (282, 110050, "Window4Region3Window", "window4_region3_321x384.png", 674, 321, groups[2]),
    )
    for key_number, window_id, caption, picture, left, width, group in specs:
        content_width = width - 20
        title, tips, values = (controls[name] for name in group)
        style_title(title, content_width)
        style_tip_list(tips, content_width)
        style_value_list(values, content_width)
        root[f"window__{key_number}"] = {
            "backgroundPic": picture,
            "beepEnable": True,
            "caption": caption,
            "id": window_id,
            "position": {"height": 384, "left": left, "top": 8, "width": width},
            f"button__{key_number + 3}": title,
            f"listview__{key_number + 6}": tips,
            f"listview__{key_number + 9}": values,
        }

    UI_PATH.write_bytes(encode_ftu(layout, header))
    print("rebuilt Window4 with three child column windows")


if __name__ == "__main__":
    main()
