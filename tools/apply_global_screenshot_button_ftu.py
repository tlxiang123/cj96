#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys
import time

from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, RESOURCE_DIR, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
BACKUP_DIR = ROOT / "Release" / f"before_global_screenshot_button_{time.strftime('%Y%m%d_%H%M%S')}"

BUTTON_ID = 20120
BUTTON_CAPTION = "GlobalScreenshotButton"
BUTTON_PIC = "global_screenshot_button_105x55.png"
BLUE_INT = 23483
BLUE = (0, 128, 255, 238)
BLUE_DARK = (0, 93, 180, 255)
WHITE_SOFT = (255, 255, 255, 42)
FONT_FAMILY = "Alibaba-PuHuiTi-Regular"


def draw_scaled(size: tuple[int, int], painter) -> Image.Image:
    scale = 4
    image = Image.new("RGBA", (size[0] * scale, size[1] * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    def p(value: int | float) -> int:
        return int(round(value * scale))

    def box(values: tuple[int | float, int | float, int | float, int | float]) -> tuple[int, int, int, int]:
        return tuple(p(value) for value in values)

    painter(draw, p, box)
    return image.resize(size, Image.Resampling.LANCZOS)


def make_screenshot_icon(path: Path) -> None:
    size = (105, 55)

    def painter(draw: ImageDraw.ImageDraw, p, box) -> None:
        width, height = size
        draw.rounded_rectangle(
            box((3, 3, width - 4, height - 4)),
            radius=p(9),
            fill=(0, 0, 0, 0),
            outline=BLUE,
            width=p(2),
        )
        draw.rounded_rectangle(
            box((7, 7, width - 8, height - 8)),
            radius=p(7),
            outline=WHITE_SOFT,
            width=p(1),
        )
        draw.rounded_rectangle(box((17, 24, 49, 40)), radius=p(3), outline=BLUE, width=p(3))
        draw.rectangle(box((23, 18, 38, 25)), outline=BLUE, width=p(3))
        draw.ellipse(box((28, 26, 40, 38)), outline=BLUE_DARK, width=p(3))
        draw.line([box((54, 31, 78, 31))[0:2], box((54, 31, 78, 31))[2:4]], fill=BLUE, width=p(4))
        draw.line([box((68, 21, 80, 31))[0:2], box((68, 21, 80, 31))[2:4]], fill=BLUE, width=p(4))
        draw.line([box((68, 41, 80, 31))[0:2], box((68, 41, 80, 31))[2:4]], fill=BLUE, width=p(4))

    draw_scaled(size, painter).save(path)


def backup() -> None:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    (BACKUP_DIR / "resources").mkdir(parents=True, exist_ok=True)
    for path in UI_DIR.glob("*.ftu"):
        shutil.copy2(path, BACKUP_DIR / "ui" / path.name)
    for name in (BUTTON_PIC, "window5_screenshot_button_105x55.png"):
        source = RESOURCE_DIR / name
        if source.is_file():
            shutil.copy2(source, BACKUP_DIR / "resources" / name)


def remove_existing_screenshot_buttons(node: object) -> None:
    captions = {"ScreenshotButton", "screenshotButton", BUTTON_CAPTION}
    ids = {BUTTON_ID, 20057}
    if isinstance(node, dict):
        for key, value in list(node.items()):
            if isinstance(value, dict) and (
                value.get("caption") in captions or value.get("id") in ids
            ):
                del node[key]
            else:
                remove_existing_screenshot_buttons(value)
    elif isinstance(node, list):
        node[:] = [
            item
            for item in node
            if not (
                isinstance(item, dict)
                and (item.get("caption") in captions or item.get("id") in ids)
            )
        ]
        for item in node:
            remove_existing_screenshot_buttons(item)


def add_global_button(data: dict) -> None:
    data["button__global_screenshot"] = {
        "caption": BUTTON_CAPTION,
        "id": BUTTON_ID,
        "position": {"left": 60, "top": 8, "width": 105, "height": 55},
        "text": "截图",
        "family": FONT_FAMILY,
        "fontSize": 20,
        "alignment": 37,
        "textPosition": {"left": 64, "top": 5, "width": 36, "height": 45},
        "iconPosition": {"left": 0, "top": 0, "width": 105, "height": 55},
        "picTab": {"pic0": BUTTON_PIC, "pic1": BUTTON_PIC, "pic2": BUTTON_PIC},
        "colorTab": {"color0": BLUE_INT, "color1": BLUE_INT, "color2": BLUE_INT},
        "touchable": True,
        "beepEnable": True,
    }


def count_global_buttons(node: object) -> int:
    if isinstance(node, dict):
        total = 1 if node.get("id") == BUTTON_ID and node.get("caption") == BUTTON_CAPTION else 0
        return total + sum(count_global_buttons(value) for value in node.values())
    if isinstance(node, list):
        return sum(count_global_buttons(value) for value in node)
    return 0


def main() -> None:
    backup()
    make_screenshot_icon(RESOURCE_DIR / BUTTON_PIC)

    updated: list[str] = []
    for path in sorted(UI_DIR.glob("*.ftu")):
        data, header, _ = decode_ftu(path)
        remove_existing_screenshot_buttons(data)
        add_global_button(data)
        path.write_bytes(encode_ftu(data, header))
        decoded, _, _ = decode_ftu(path)
        count = count_global_buttons(decoded)
        if count != 1:
            raise RuntimeError(f"{path.name}: expected exactly one global screenshot button, got {count}")
        updated.append(path.name)

    print(f"updated global screenshot button in {len(updated)} ftu files: {', '.join(updated)}")
    print(f"backup={BACKUP_DIR}")


if __name__ == "__main__":
    main()
