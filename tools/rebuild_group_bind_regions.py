#!/usr/bin/env python3
"""Rebuild GroupBindValueWindow as three columns plus a bottom action row."""

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
UI_PATH = ROOT / "ui" / "main.ftu"
LABEL_SOURCE = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-8a32cca9-8b3b-4785-8e14-72ec73b5f05c.png")
SELECT_SOURCE = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-7b90d123-10b0-4a68-819d-edd737db4378.png")

PANELS = {
    "w2_group_bind_region1_405x305.png": (405, 305),
    "w2_group_bind_region2_245x305.png": (245, 305),
    "w2_group_bind_region3_309x305.png": (309, 305),
    "w2_group_bind_action_983x70.png": (983, 70),
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


def build_icon_asset(source_path: Path, crop: tuple[int, int, int, int], output: str,
                     size: tuple[int, int], radius: int) -> None:
    width, height = size
    scale = 4
    image = Image.new("RGBA", (width * scale, height * scale), (235, 247, 255, 255))
    gradient = Image.new("RGBA", image.size)
    pixels = gradient.load()
    for y in range(height * scale):
        ratio = y / max(1, height * scale - 1)
        color = (round(250 - 19 * ratio), round(253 - 8 * ratio), 255, 255)
        for x in range(width * scale):
            pixels[x, y] = color
    mask = Image.new("L", image.size, 0)
    draw_mask = ImageDraw.Draw(mask)
    draw_mask.rounded_rectangle(
        (1 * scale, 1 * scale, (width - 2) * scale, (height - 2) * scale),
        radius=max(9, height // 6) * scale,
        fill=255,
    )
    image.paste(gradient, (0, 0), mask)
    ImageDraw.Draw(image).rounded_rectangle(
        (1 * scale, 1 * scale, (width - 2) * scale, (height - 2) * scale),
        radius=max(9, height // 6) * scale,
        outline=(18, 145, 235, 255),
        width=2 * scale,
    )
    result = image.resize((width, height), Image.Resampling.LANCZOS)

    source = Image.open(source_path).convert("RGB").crop(crop).resize(
        (width, height), Image.Resampling.LANCZOS
    )
    source_px = source.load()
    result_px = result.load()
    margin_x = max(6, width // 24)
    margin_y = max(6, height // 16)
    for y in range(margin_y, height - margin_y):
        for x in range(margin_x, width - margin_x):
            r, g, b = source_px[x, y]
            if b > 110 and b - r > 24 and b - g > 8:
                result_px[x, y] = (r, g, b, 255)
    result.save(RESOURCES / output, format="PNG", optimize=True)


def build_icon_assets() -> None:
    specs = (
        (LABEL_SOURCE, (45, 9, 421, 194), "w2_bind_group_number_145x70.png", (145, 70), 40),
        (LABEL_SOURCE, (45, 228, 421, 410), "w2_bind_group_name_145x70.png", (145, 70), 40),
        (LABEL_SOURCE, (45, 449, 421, 630), "w2_bind_linked_pump_145x70.png", (145, 70), 40),
        (SELECT_SOURCE, (70, 8, 448, 188), "w2_bind_linked_sensor_145x70.png", (145, 70), 40),
        (SELECT_SOURCE, (70, 226, 448, 407), "w2_bind_select_pump_215x105.png", (215, 105), 40),
        (SELECT_SOURCE, (70, 445, 448, 627), "w2_bind_select_sensor_215x105.png", (215, 105), 40),
    )
    for spec in specs:
        build_icon_asset(*spec)


def set_position(control: dict, left: int, top: int, width: int, height: int) -> None:
    control["position"] = {"height": height, "left": left, "top": top, "width": width}


def style_label(control: dict, left: int, top: int, picture: str) -> None:
    set_position(control, left, top, 145, 70)
    control["visible"] = True
    control["touchable"] = False
    control.pop("text", None)
    control.pop("bgColorTab", None)
    control["backgroundPic"] = picture


def style_value(control: dict, left: int, top: int) -> None:
    set_position(control, left, top, 220, 41)
    control["visible"] = True
    control["alignment"] = 37
    control["fontSize"] = 22
    control["touchable"] = False
    control["bgColorTab"] = {"color0": 16120575}
    control["colorTab"] = {"color0": 23483}


def style_list(control: dict, left: int, width: int) -> None:
    set_position(control, left, 120, width, 170)
    control["visible"] = True
    control["rows"] = 3
    control["cols"] = 2
    control["rowSpacing"] = 5
    control["colSpacing"] = 5
    item = control["item"]
    item_width = (width - 5) // 2
    set_position(item, 0, 0, item_width, 52)
    item["alignment"] = 36
    item["fontSize"] = 17
    item["backgroundColor"] = 16186367
    item["bgColorTab"] = {
        "color0": 16186367,
        "color1": 14479087,
        "color2": 14479087,
    }
    item["colorTab"] = {"color0": 23483, "color1": 23483, "color2": 23483}
    item["picTab"] = {
        "pic0": "window7_select_off.png",
        "pic2": "window7_select_on.png",
    }
    item["iconPosition"] = {
        "height": 28,
        "left": item_width - 34,
        "top": 12,
        "width": 28,
    }


def main() -> None:
    for name, (width, height) in PANELS.items():
        build_panel(RESOURCES / name, width, height)
    build_icon_assets()

    layout, header, _ = decode_ftu(UI_PATH)
    root = find_caption(layout, "GroupBindValueWindow")
    captions = (
        "TextView4", "GroupNumEditText", "TextView5", "EditText1",
        "TextView6", "AddPumpEditText", "TextView7", "AddSenserEditText",
        "SelectPumpButton", "SelectPumpListView",
        "SelectSenserButton", "SelectSenserListView",
        "GroupAddButton", "GroupCencelButton",
    )
    controls = {caption: pop_caption(root, caption) for caption in captions}

    for key, value in list(root.items()):
        if isinstance(value, dict) and value.get("caption", "").startswith("GroupBindRegion"):
            del root[key]

    root["backgroundColor"] = 13624314
    root["visible"] = False
    root["position"] = {"height": 400, "left": 8, "top": 93, "width": 1007}

    style_label(controls["TextView4"], 10, 6, "w2_bind_group_number_145x70.png")
    style_value(controls["GroupNumEditText"], 165, 20)
    style_label(controls["TextView5"], 10, 80, "w2_bind_group_name_145x70.png")
    style_value(controls["EditText1"], 165, 94)
    style_label(controls["TextView6"], 10, 154, "w2_bind_linked_pump_145x70.png")
    style_value(controls["AddPumpEditText"], 165, 168)
    style_label(controls["TextView7"], 10, 228, "w2_bind_linked_sensor_145x70.png")
    style_value(controls["AddSenserEditText"], 165, 242)

    set_position(controls["SelectPumpButton"], 15, 7, 215, 105)
    controls["SelectPumpButton"]["visible"] = True
    controls["SelectPumpButton"].pop("text", None)
    controls["SelectPumpButton"]["picTab"] = {
        "pic0": "w2_bind_select_pump_215x105.png",
        "pic2": "w2_bind_select_pump_215x105.png",
    }
    style_list(controls["SelectPumpListView"], 10, 225)

    set_position(controls["SelectSenserButton"], 47, 7, 215, 105)
    controls["SelectSenserButton"]["visible"] = True
    controls["SelectSenserButton"].pop("text", None)
    controls["SelectSenserButton"]["picTab"] = {
        "pic0": "w2_bind_select_sensor_215x105.png",
        "pic2": "w2_bind_select_sensor_215x105.png",
    }
    style_list(controls["SelectSenserListView"], 10, 289)

    add = controls["GroupAddButton"]
    add.pop("text", None)
    add["picTab"] = {
        "pic0": "w2_group_bind_confirm_120x60.png",
        "pic2": "w2_group_bind_confirm_120x60_pressed.png",
    }
    set_position(add, 543, 5, 120, 60)

    cancel = controls["GroupCencelButton"]
    cancel.pop("text", None)
    cancel["picTab"] = {
        "pic0": "w2_group_bind_cancel_120x60.png",
        "pic2": "w2_group_bind_cancel_120x60_pressed.png",
    }
    set_position(cancel, 320, 5, 120, 60)

    root["window__260"] = {
        "backgroundPic": "w2_group_bind_region1_405x305.png",
        "beepEnable": True,
        "caption": "GroupBindRegion1Window",
        "id": 110044,
        "position": {"height": 305, "left": 12, "top": 8, "width": 405},
        "textview__264": controls["TextView4"],
        "edittext__265": controls["GroupNumEditText"],
        "textview__266": controls["TextView5"],
        "edittext__267": controls["EditText1"],
        "textview__268": controls["TextView6"],
        "edittext__269": controls["AddPumpEditText"],
        "textview__270": controls["TextView7"],
        "edittext__271": controls["AddSenserEditText"],
    }
    root["window__261"] = {
        "backgroundPic": "w2_group_bind_region2_245x305.png",
        "beepEnable": True,
        "caption": "GroupBindRegion2Window",
        "id": 110045,
        "position": {"height": 305, "left": 429, "top": 8, "width": 245},
        "button__272": controls["SelectPumpButton"],
        "listview__273": controls["SelectPumpListView"],
    }
    root["window__262"] = {
        "backgroundPic": "w2_group_bind_region3_309x305.png",
        "beepEnable": True,
        "caption": "GroupBindRegion3Window",
        "id": 110046,
        "position": {"height": 305, "left": 686, "top": 8, "width": 309},
        "button__274": controls["SelectSenserButton"],
        "listview__275": controls["SelectSenserListView"],
    }
    root["window__263"] = {
        "backgroundPic": "w2_group_bind_action_983x70.png",
        "beepEnable": True,
        "caption": "GroupBindRegion4Window",
        "id": 110047,
        "position": {"height": 70, "left": 12, "top": 321, "width": 983},
        "button__276": cancel,
        "button__277": add,
    }

    UI_PATH.write_bytes(encode_ftu(layout, header))
    print("rebuilt GroupBindValueWindow with four child windows")


if __name__ == "__main__":
    main()
