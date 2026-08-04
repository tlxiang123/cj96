#!/usr/bin/env python3
from __future__ import annotations

import shutil
import sys
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
import ftu_style  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / "ui"
RES = ROOT / "resources"
MAIN_FTU = UI / "main.ftu"
TRASH_REF = Path(
    r"C:\Users\Administrator\AppData\Local\Temp"
    r"\codex-clipboard-6d802caa-14a6-435d-a0a1-1e0f7dd7f5be.png"
)

BUTTON_W = 120
BUTTON_H = 60
SCALE = 4

BORDER = (0, 135, 233, 255)
BLUE = (0, 91, 187, 255)
FILL_TOP = (252, 254, 255, 255)
FILL_BOTTOM = (238, 248, 255, 255)


def backup_files() -> Path:
    backup = ROOT / "Release" / (
        "set_runtime_buttons_same_font_backup_"
        + datetime.now().strftime("%Y%m%d_%H%M%S")
    )
    backup.mkdir(parents=True, exist_ok=True)
    for path in [
        MAIN_FTU,
        UI / "w2_group_bind_cancel_120x60.png",
        UI / "set_runtime_clear_trash_ref_120x60.png",
        UI / "set_runtime_ok_match_cancel_120x60.png",
    ]:
        if path.exists():
            shutil.copy2(path, backup / path.name)
    return backup


def font() -> ImageFont.FreeTypeFont:
    candidates = [
        Path(r"C:\Windows\Fonts\msyhbd.ttc"),
        Path(r"C:\Windows\Fonts\simhei.ttf"),
        ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf",
    ]
    for candidate in candidates:
        if candidate.exists():
            return ImageFont.truetype(str(candidate), 21 * SCALE)
    raise RuntimeError("No usable Chinese font found")


FONT = font()


def base_button() -> Image.Image:
    im = Image.new("RGBA", (BUTTON_W * SCALE, BUTTON_H * SCALE), (0, 0, 0, 0))
    mask = Image.new("L", im.size, 0)
    box = [1 * SCALE, 1 * SCALE, (BUTTON_W - 2) * SCALE, (BUTTON_H - 2) * SCALE]
    ImageDraw.Draw(mask).rounded_rectangle(box, radius=7 * SCALE, fill=255)

    gradient = Image.new("RGBA", im.size, (0, 0, 0, 0))
    pix = gradient.load()
    for y in range(im.height):
        t = y / max(1, im.height - 1)
        color = tuple(int(FILL_TOP[i] * (1 - t) + FILL_BOTTOM[i] * t) for i in range(4))
        for x in range(im.width):
            pix[x, y] = color

    im.alpha_composite(Image.composite(gradient, Image.new("RGBA", im.size), mask))
    ImageDraw.Draw(im).rounded_rectangle(
        box, radius=7 * SCALE, outline=BORDER, width=2 * SCALE
    )
    return im


def blue_component_icon(src: Image.Image, crop: tuple[int, int, int, int],
                        max_size: tuple[int, int]) -> Image.Image:
    cropped = src.crop(crop).convert("RGBA")
    cropped = cropped.resize(
        (cropped.width * SCALE, cropped.height * SCALE),
        Image.Resampling.LANCZOS,
    )
    alpha = Image.new("L", cropped.size, 0)
    src_pix = cropped.load()
    alpha_pix = alpha.load()
    for y in range(cropped.height):
        for x in range(cropped.width):
            r, g, b, a = src_pix[x, y]
            is_blue = (
                a > 0
                and b > 90
                and b >= g + 3
                and b >= r + 20
                and r < 150
            )
            if is_blue:
                alpha_pix[x, y] = max(70, min(255, int((b - r) * 2.5)))

    alpha = alpha.filter(ImageFilter.GaussianBlur(0.15 * SCALE))
    bbox = alpha.getbbox()
    if bbox:
        alpha = alpha.crop(bbox)

    icon = Image.new("RGBA", alpha.size, BLUE)
    icon.putalpha(alpha)
    icon.thumbnail(max_size, Image.Resampling.LANCZOS)
    return icon


def draw_text(draw: ImageDraw.ImageDraw, text: str) -> None:
    bbox = draw.textbbox((0, 0), text, font=FONT)
    tw = bbox[2] - bbox[0]
    th = bbox[3] - bbox[1]
    # Leave a fixed icon area so three labels line up consistently.
    x = 55 * SCALE
    y = (BUTTON_H * SCALE - th) // 2 - 1 * SCALE
    draw.text((x, y), text, font=FONT, fill=BLUE)


def save_button(name: str, icon: Image.Image, text: str) -> None:
    im = base_button()
    im.alpha_composite(icon, (15 * SCALE, (BUTTON_H * SCALE - icon.height) // 2))
    draw_text(ImageDraw.Draw(im), text)
    out = im.resize((BUTTON_W, BUTTON_H), Image.Resampling.LANCZOS)
    for folder in [UI, RES]:
        path = folder / name
        out.save(path, optimize=True)
        if Image.open(path).size != (BUTTON_W, BUTTON_H):
            raise RuntimeError(f"{path} is not 120x60")


def walk(node):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def find_caption(data: dict, caption: str) -> dict:
    for node in walk(data):
        if isinstance(node, dict) and node.get("caption") == caption:
            return node
    raise RuntimeError(f"{caption} not found")


def update_ftu() -> None:
    data, header, _ = ftu_style.decode_ftu(MAIN_FTU)
    mapping = {
        "SetRunTimeCencelButton": "set_runtime_cancel_same_font_120x60.png",
        "SetRunTimeZeroButton": "set_runtime_clear_same_font_120x60.png",
        "SetRunTimeOKButton": "set_runtime_ok_same_font_120x60.png",
    }
    for caption, image_name in mapping.items():
        node = find_caption(data, caption)
        node["position"]["width"] = BUTTON_W
        node["position"]["height"] = BUTTON_H
        node["picTab"] = {"pic0": image_name, "pic1": image_name, "pic2": image_name}
        node["iconPosition"] = {"left": 0, "top": 0, "width": BUTTON_W, "height": BUTTON_H}
        node["textPosition"] = {"left": 0, "top": 0, "width": BUTTON_W, "height": BUTTON_H}
        node["text"] = None
        node["colorTab"] = {"color0": 23483, "color1": 23483, "color2": 23483}
        node.pop("backgroundPic", None)
    MAIN_FTU.write_bytes(ftu_style.encode_ftu(data, header))


def main() -> None:
    if not TRASH_REF.exists():
        raise RuntimeError(f"trash reference image missing: {TRASH_REF}")
    backup = backup_files()

    cancel_src = Image.open(UI / "w2_group_bind_cancel_120x60.png").convert("RGBA")
    ok_src = Image.open(UI / "w2_ok_120x60.png").convert("RGBA")
    trash_src = Image.open(TRASH_REF).convert("RGBA")

    cancel_icon = blue_component_icon(cancel_src, (8, 12, 49, 47), (34 * SCALE, 34 * SCALE))
    ok_icon = blue_component_icon(ok_src, (10, 13, 48, 44), (34 * SCALE, 34 * SCALE))
    trash_icon = blue_component_icon(
        trash_src, (135, 45, 275, 205), (34 * SCALE, 38 * SCALE)
    )

    save_button("set_runtime_cancel_same_font_120x60.png", cancel_icon, "取消")
    save_button("set_runtime_clear_same_font_120x60.png", trash_icon, "清除")
    save_button("set_runtime_ok_same_font_120x60.png", ok_icon, "确认")
    update_ftu()

    data, _, _ = ftu_style.decode_ftu(MAIN_FTU)
    for caption in [
        "SetRunTimeCencelButton",
        "SetRunTimeZeroButton",
        "SetRunTimeOKButton",
    ]:
        node = find_caption(data, caption)
        print(caption, node["position"], node["picTab"], node["iconPosition"])
    print("backup", backup)


if __name__ == "__main__":
    main()
