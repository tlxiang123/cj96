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
BACKUP_DIR = ROOT / "Release" / f"before_window5_screenshot_button_{time.strftime('%Y%m%d_%H%M%S')}"

BLUE_INT = 23483
BLUE = (0, 128, 255, 238)
BLUE_DARK = (0, 93, 180, 255)
WHITE_SOFT = (255, 255, 255, 42)
FONT_FAMILY = "Alibaba-PuHuiTi-Regular"

LEFT_REGION = "window5_region1_frame_430x340.png"
RIGHT_REGION = "window5_region2_frame_405x340.png"
SCREENSHOT_ICON = "window5_screenshot_button_105x55.png"


def find_caption(node: object, caption: str) -> dict | None:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            found = find_caption(value, caption)
            if found is not None:
                return found
    elif isinstance(node, list):
        for value in node:
            found = find_caption(value, caption)
            if found is not None:
                return found
    return None


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


def make_region(path: Path, size: tuple[int, int]) -> None:
    def painter(draw: ImageDraw.ImageDraw, p, box) -> None:
        width, height = size
        draw.rounded_rectangle(
            box((2, 2, width - 3, height - 3)),
            radius=p(10),
            fill=(255, 255, 255, 34),
            outline=BLUE,
            width=p(2),
        )
        draw.rounded_rectangle(
            box((7, 7, width - 8, height - 8)),
            radius=p(7),
            outline=WHITE_SOFT,
            width=p(1),
        )

    draw_scaled(size, painter).save(path)


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
    shutil.copy2(UI_DIR / "main.ftu", BACKUP_DIR / "ui" / "main.ftu")
    for name in (LEFT_REGION, RIGHT_REGION, SCREENSHOT_ICON):
        source = RESOURCE_DIR / name
        if source.is_file():
            shutil.copy2(source, BACKUP_DIR / "resources" / name)


def strip_window5_visual_helpers(window5: dict) -> list[tuple[str, object]]:
    removed_captions = {
        "Window5LeftRegion",
        "Window5RightRegion",
        "Window5Region1Window",
        "Window5Region2Window",
        "Window5Region1Frame",
        "Window5Region2Frame",
        "ScreenshotButton",
        "screenshotButton",
    }
    removed_ids = {110063, 110064, 50091, 50092, 20120, 20057}
    body: list[tuple[str, object]] = []
    for key, value in window5.items():
        if isinstance(value, dict) and (
            value.get("caption") in removed_captions or value.get("id") in removed_ids
        ):
            continue
        body.append((key, value))
    return body


def ensure_window5_helpers(data: dict) -> None:
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found")

    metadata_keys = {
        "backgroundColor",
        "backgroundPic",
        "beepEnable",
        "caption",
        "id",
        "position",
        "visible",
        "touchable",
        "text",
        "colorTab",
        "bgColorTab",
        "resolution",
        "topmost",
    }
    body = strip_window5_visual_helpers(window5)
    metadata = [(key, value) for key, value in body if key in metadata_keys]
    controls = [(key, value) for key, value in body if key not in metadata_keys]

    window5.clear()
    window5.update(metadata)
    window5["textview__window5_region1_frame"] = {
        "caption": "Window5Region1Frame",
        "id": 50091,
        "position": {"left": 30, "top": 27, "width": 430, "height": 340},
        "backgroundPic": LEFT_REGION,
        "text": "",
        "touchable": False,
        "beepEnable": False,
    }
    window5["textview__window5_region2_frame"] = {
        "caption": "Window5Region2Frame",
        "id": 50092,
        "position": {"left": 575, "top": 27, "width": 405, "height": 340},
        "backgroundPic": RIGHT_REGION,
        "text": "",
        "touchable": False,
        "beepEnable": False,
    }
    window5.update(controls)
    window5["button__window5_screenshot"] = {
        "caption": "ScreenshotButton",
        "id": 20120,
        "position": {"left": 467, "top": 306, "width": 105, "height": 55},
        "text": "截图",
        "family": FONT_FAMILY,
        "fontSize": 20,
        "alignment": 37,
        "textPosition": {"left": 64, "top": 5, "width": 36, "height": 45},
        "iconPosition": {"left": 0, "top": 0, "width": 105, "height": 55},
        "picTab": {"pic0": SCREENSHOT_ICON, "pic1": SCREENSHOT_ICON, "pic2": SCREENSHOT_ICON},
        "colorTab": {"color0": BLUE_INT, "color1": BLUE_INT, "color2": BLUE_INT},
        "touchable": True,
        "beepEnable": True,
    }


def verify(data: dict) -> None:
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found after write")
    required = {
        "Window5Region1Frame": 50091,
        "Window5Region2Frame": 50092,
        "ScreenshotButton": 20120,
    }
    for caption, control_id in required.items():
        node = find_caption(window5, caption)
        if node is None:
            raise RuntimeError(f"{caption} missing")
        if node.get("id") != control_id:
            raise RuntimeError(f"{caption} id mismatch: {node.get('id')} != {control_id}")
    shot = find_caption(window5, "ScreenshotButton")
    if shot.get("text") != "截图":
        raise RuntimeError("screenshot button text mismatch")


def main() -> None:
    backup()
    make_region(RESOURCE_DIR / LEFT_REGION, (430, 340))
    make_region(RESOURCE_DIR / RIGHT_REGION, (405, 340))
    make_screenshot_icon(RESOURCE_DIR / SCREENSHOT_ICON)

    ftu_path = UI_DIR / "main.ftu"
    data, header, _ = decode_ftu(ftu_path)
    ensure_window5_helpers(data)
    ftu_path.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(ftu_path)
    verify(decoded)
    print(f"updated Window5 IDE-visible frames and screenshot button; backup={BACKUP_DIR}")


if __name__ == "__main__":
    main()
