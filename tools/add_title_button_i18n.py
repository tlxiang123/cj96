#!/usr/bin/env python3
"""Generate localized LX96 title-button images.

The Chinese subtitle is baked into resources/set_button.png. This tool rebuilds
only the subtitle area for each language while preserving the droplet logo and
the LX96 wordmark pixel-for-pixel from the original, then writes
set_button_xx.png (320x80, RGB) next to set_button.png.

Usage:
  python -X utf8 tools/add_title_button_i18n.py --check
  python -X utf8 tools/add_title_button_i18n.py
"""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "resources" / "set_button.png"
FONT = ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf"
OUT_DIR = ROOT / "resources"
DIAG_DIR = ROOT / "diagnostics"

TEXT_COLOR = (20, 84, 197)
GRADIENT_X = 300
CLEAR_FROM = 158
CLEAR_TO = 311
TEXT_RIGHT = 278
TEXT_CENTER_Y = 40.5

TITLES = [
    ("en", "Controller", 22),
    ("de", "Steuerung", 22),
    ("fr", "Contr\u00f4leur", 22),
    ("ja", "\u30b3\u30f3\u30c8\u30ed\u30fc\u30e9", 21),
]


def build_base(src):
    base = src.copy()
    px = base.load()
    height = base.size[1]
    for y in range(height):
        column = px[GRADIENT_X, y]
        for x in range(CLEAR_FROM, CLEAR_TO + 1):
            px[x, y] = column
    return base


def render(base, text, size):
    image = base.copy()
    draw = ImageDraw.Draw(image)
    font = ImageFont.truetype(str(FONT), size)
    bbox = font.getbbox(text)
    width = bbox[2] - bbox[0]
    height = bbox[3] - bbox[1]
    x = TEXT_RIGHT - width - bbox[0]
    y = int(round(TEXT_CENTER_Y - (bbox[1] + height / 2.0)))
    draw.text((x, y), text, font=font, fill=TEXT_COLOR,
              stroke_width=1, stroke_fill=TEXT_COLOR)
    return image


def check(image):
    problems = []
    if image.size != (320, 80):
        problems.append("size " + str(image.size))
    if image.mode != "RGB":
        problems.append("mode " + image.mode)
    px = image.load()
    for y in range(80):
        for x in range(CLEAR_FROM, CLEAR_TO + 1):
            if px[x, y] == (0, 0, 0):
                problems.append("black pixel at " + str(x) + "," + str(y))
                return problems
    return problems


def main():
    check_only = "--check" in sys.argv
    if not SRC.exists():
        print("missing " + str(SRC))
        return 1
    if not FONT.exists():
        print("missing " + str(FONT))
        return 1
    src = Image.open(SRC).convert("RGB")
    base = build_base(src)
    DIAG_DIR.mkdir(exist_ok=True)
    base.save(DIAG_DIR / "set_button_base_no_title.png")
    bad = 0
    for key, text, size in TITLES:
        image = render(base, text, size)
        out = OUT_DIR / ("set_button_" + key + ".png")
        problems = check(image)
        if problems:
            bad += 1
            print(out.name + "  FAIL  " + "; ".join(problems))
            continue
        if check_only:
            print(out.name + "  ok")
            continue
        image.save(out)
        print(out.name + "  written  " + str(out.stat().st_size) + " bytes")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())
