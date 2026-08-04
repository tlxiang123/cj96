#!/usr/bin/env python3
"""Configure Window9 as the W2 clear-scope dialog and build its button art."""

from pathlib import Path
import shutil
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
MAIN_FTU = ROOT / "ui" / "main.ftu"
FONT = ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf"
DIALOG_BACKGROUND = ROOT / "ui" / "w2_group_rename_dialog_560x220.png"
SYSTEM_BOLD_FONT = Path("C:/Windows/Fonts/msyhbd.ttc")


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


def build_button(path: Path, label: str, icon: str, left: int, top: int) -> None:
    scale = 4
    width, height = 120, 60
    dialog = Image.open(DIALOG_BACKGROUND).convert("RGBA")
    image = dialog.crop((left, top, left + width, top + height)).resize(
        (width * scale, height * scale), Image.Resampling.LANCZOS
    )

    button_fill = Image.new("RGBA", image.size, (0, 0, 0, 0))
    fill_draw = ImageDraw.Draw(button_fill)

    for y in range(2 * scale, (height - 2) * scale):
        ratio = (y - 2 * scale) / ((height - 4) * scale)
        color = (
            round(252 - 21 * ratio),
            round(255 - 13 * ratio),
            255,
            255,
        )
        fill_draw.line((2 * scale, y, (width - 2) * scale, y), fill=color)

    button_mask = Image.new("L", image.size, 0)
    ImageDraw.Draw(button_mask).rounded_rectangle(
        (2 * scale, 2 * scale, (width - 3) * scale, (height - 3) * scale),
        radius=10 * scale,
        fill=255,
    )
    image.alpha_composite(Image.composite(button_fill, Image.new("RGBA", image.size), button_mask))
    draw = ImageDraw.Draw(image)

    blue = (22, 116, 205, 255)
    draw.rounded_rectangle(
        (2 * scale, 2 * scale, (width - 3) * scale, (height - 3) * scale),
        radius=10 * scale,
        outline=blue,
        width=2 * scale,
    )

    icon_box = (12 * scale, 15 * scale, 40 * scale, 43 * scale)
    if icon == "pin":
        cx, cy = 26 * scale, 25 * scale
        draw.ellipse((17 * scale, 15 * scale, 35 * scale, 33 * scale), outline=blue, width=2 * scale)
        draw.ellipse((23 * scale, 21 * scale, 29 * scale, 27 * scale), outline=blue, width=2 * scale)
        draw.line((18 * scale, 29 * scale, cx, 42 * scale, 34 * scale, 29 * scale), fill=blue, width=2 * scale)
    else:
        draw.rounded_rectangle(icon_box, radius=4 * scale, outline=blue, width=2 * scale)
        draw.line((18 * scale, 29 * scale, 24 * scale, 35 * scale, 35 * scale, 22 * scale), fill=blue, width=3 * scale)

    font_path = SYSTEM_BOLD_FONT if SYSTEM_BOLD_FONT.exists() else FONT
    font = ImageFont.truetype(str(font_path), 22 * scale)
    box = draw.textbbox((0, 0), label, font=font)
    text_height = box[3] - box[1]
    draw.text(
        (48 * scale, (height * scale - text_height) // 2 - box[1]),
        label,
        font=font,
        fill=blue,
    )
    image.resize((width, height), Image.Resampling.LANCZOS).save(path, optimize=True)


def main() -> None:
    current_name = "w2_clear_current_120x60.png"
    all_name = "w2_clear_all_120x60.png"
    build_button(ROOT / "resources" / current_name, "当前", "pin", 196, 141)
    build_button(ROOT / "resources" / all_name, "全部", "check", 340, 141)
    shutil.copy2(ROOT / "resources" / current_name, ROOT / "ui" / current_name)
    shutil.copy2(ROOT / "resources" / all_name, ROOT / "ui" / all_name)

    layout, header, _ = decode_ftu(MAIN_FTU)
    window = find_caption(layout, "Window9")
    window["visible"] = False

    button8 = find_caption(window, "Button8")
    button8["position"] = {"height": 60, "left": 196, "top": 141, "width": 120}
    button8["picTab"] = {"pic0": current_name, "pic1": current_name, "pic2": current_name}
    button8.pop("text", None)

    button44 = find_caption(window, "Button44")
    button44["position"] = {"height": 60, "left": 340, "top": 141, "width": 120}
    button44["picTab"] = {"pic0": all_name, "pic1": all_name, "pic2": all_name}
    button44.pop("text", None)

    for key, value in list(window.items()):
        if isinstance(value, dict) and value.get("caption") == "ClearIrrPromptTextView":
            del window[key]
    window["textview__223"] = {
        "alignment": 37,
        "bold": True,
        "caption": "ClearIrrPromptTextView",
        "colorTab": {"color0": 23483},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": 24,
        "id": 50065,
        "position": {"height": 105, "left": 30, "top": 20, "width": 500},
        "text": "清空当前选中的阀组，\n还是清空全部阀组？",
    }

    MAIN_FTU.write_bytes(encode_ftu(layout, header))
    print("configured Window9 clear-scope dialog")


if __name__ == "__main__":
    main()
