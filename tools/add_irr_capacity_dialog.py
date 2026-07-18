#!/usr/bin/env python3
"""Add the W2 valve-capacity picker and style Button43."""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
UI_PATH = ROOT / "ui" / "main.ftu"
SOURCE = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-e663751c-9d9d-4608-bac3-9db6311c353b.png")
ONE_CLICK = RESOURCES / "w2_one_click_add_120x60.png"


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


def build_one_click_image() -> None:
    scale = 4
    width, height = 120, 60
    image = Image.new("RGBA", (width * scale, height * scale), (235, 247, 255, 255))
    gradient = Image.new("RGBA", image.size)
    pixels = gradient.load()
    for y in range(height * scale):
        ratio = y / max(1, height * scale - 1)
        color = (round(250 - 19 * ratio), round(253 - 8 * ratio), 255, 255)
        for x in range(width * scale):
            pixels[x, y] = color
    mask = Image.new("L", image.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (1 * scale, 1 * scale, 118 * scale, 58 * scale),
        radius=10 * scale,
        fill=255,
    )
    image.paste(gradient, (0, 0), mask)
    draw = ImageDraw.Draw(image)
    blue = (0, 105, 210)
    draw.rounded_rectangle(
        (1 * scale, 1 * scale, 118 * scale, 58 * scale),
        radius=10 * scale,
        outline=(18, 145, 235),
        width=2 * scale,
    )
    draw.rounded_rectangle(
        (11 * scale, 27 * scale, 39 * scale, 33 * scale),
        radius=2 * scale,
        fill=blue,
    )
    draw.rounded_rectangle(
        (22 * scale, 16 * scale, 28 * scale, 44 * scale),
        radius=2 * scale,
        fill=blue,
    )
    font = ImageFont.truetype(str(ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf"), 19 * scale)
    text = "一键添加"
    box = draw.textbbox((0, 0), text, font=font)
    text_height = box[3] - box[1]
    draw.text((44 * scale, (height * scale - text_height) // 2 - box[1]), text, font=font, fill=(0, 81, 198))
    image.resize((width, height), Image.Resampling.LANCZOS).save(ONE_CLICK, format="PNG", optimize=True)


def build_transparent_dialog() -> None:
    scale = 4
    width, height = 560, 220
    image = Image.new("RGBA", (width * scale, height * scale), (0, 0, 0, 0))
    gradient = Image.new("RGBA", image.size)
    pixels = gradient.load()
    for y in range(height * scale):
        ratio = y / max(1, height * scale - 1)
        color = (round(254 - 8 * ratio), round(255 - 2 * ratio), 255, 255)
        for x in range(width * scale):
            pixels[x, y] = color
    mask = Image.new("L", image.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (3 * scale, 3 * scale, (width - 4) * scale, (height - 4) * scale),
        radius=16 * scale,
        fill=255,
    )
    image.paste(gradient, (0, 0), mask)
    ImageDraw.Draw(image).rounded_rectangle(
        (3 * scale, 3 * scale, (width - 4) * scale, (height - 4) * scale),
        radius=16 * scale,
        outline=(14, 151, 232, 255),
        width=3 * scale,
    )
    image.resize((width, height), Image.Resampling.LANCZOS).save(
        RESOURCES / "w2_group_rename_dialog_560x220.png",
        format="PNG",
        optimize=True,
    )


def build_capacity_selected_images() -> None:
    for capacity in (2, 3, 4):
        scale = 4
        size = 69
        font = ImageFont.truetype(str(ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf"), 42 * scale)
        for selected in (False, True):
            image = Image.new("RGBA", (size * scale, size * scale), (0, 0, 0, 0))
            draw = ImageDraw.Draw(image)
            border = (35, 174, 93, 255) if selected else (14, 151, 232, 255)
            draw.rounded_rectangle(
                (3 * scale, 3 * scale, (size - 4) * scale, (size - 4) * scale),
                radius=9 * scale,
                fill=(248, 253, 255, 255),
                outline=border,
                width=3 * scale,
            )
            text = str(capacity)
            box = draw.textbbox((0, 0), text, font=font)
            text_width = box[2] - box[0]
            text_height = box[3] - box[1]
            draw.text(
                ((size * scale - text_width) // 2, (size * scale - text_height) // 2 - box[1]),
                text,
                font=font,
                fill=(0, 81, 198, 255),
            )
            suffix = "_selected" if selected else ""
            image.resize((size, size), Image.Resampling.LANCZOS).save(
                RESOURCES / f"w2_capacity_{capacity}_69x69{suffix}.png",
                format="PNG",
                optimize=True,
            )


def add_dialog() -> None:
    layout, header, _ = decode_ftu(UI_PATH)
    w2 = find_caption(layout, "w2set_window")
    button43 = find_caption(w2, "Button43")
    button43.pop("text", None)
    button43["picTab"] = {"pic0": ONE_CLICK.name, "pic2": ONE_CLICK.name}

    for key, value in list(w2.items()):
        if isinstance(value, dict) and value.get("caption") == "IrrCapacityWindow":
            del w2[key]

    used = {key for key in w2 if key.startswith("window__")}
    number = 250
    while f"window__{number}" in used:
        number += 1

    w2[f"window__{number}"] = {
        "backgroundPic": "w2_group_rename_dialog_560x220.png",
        "beepEnable": True,
        "caption": "IrrCapacityWindow",
        "id": 110043,
        "visible": False,
        "position": {"height": 220, "left": 224, "top": 95, "width": 560},
        "button__251": {
            "alignment": 37,
            "caption": "IrrCapacity2Button",
            "id": 20181,
            "picTab": {"pic0": "w2_capacity_2_69x69.png", "pic2": "w2_capacity_2_69x69_selected.png"},
            "position": {"height": 69, "left": 110, "top": 24, "width": 69},
        },
        "button__252": {
            "alignment": 37,
            "caption": "IrrCapacity3Button",
            "id": 20182,
            "picTab": {"pic0": "w2_capacity_3_69x69.png", "pic2": "w2_capacity_3_69x69_selected.png"},
            "position": {"height": 69, "left": 245, "top": 24, "width": 69},
        },
        "button__253": {
            "alignment": 37,
            "caption": "IrrCapacity4Button",
            "id": 20183,
            "picTab": {"pic0": "w2_capacity_4_69x69.png", "pic2": "w2_capacity_4_69x69_selected.png"},
            "position": {"height": 69, "left": 380, "top": 24, "width": 69},
        },
        "button__254": {
            "alignment": 37,
            "caption": "IrrCapacityCancelButton",
            "id": 20184,
            "picTab": {"pic0": "w2_group_bind_cancel_120x60.png", "pic2": "w2_group_bind_cancel_120x60.png"},
            "position": {"height": 60, "left": 145, "top": 145, "width": 120},
        },
        "button__255": {
            "alignment": 37,
            "caption": "IrrCapacityOkButton",
            "id": 20185,
            "picTab": {"pic0": "w2_group_rename_confirm_120x60.png", "pic2": "w2_group_rename_confirm_120x60.png"},
            "position": {"height": 60, "left": 295, "top": 145, "width": 120},
        },
    }
    UI_PATH.write_bytes(encode_ftu(layout, header))
    print(f"added IrrCapacityWindow as window__{number}")


if __name__ == "__main__":
    build_one_click_image()
    build_transparent_dialog()
    build_capacity_selected_images()
    add_dialog()
