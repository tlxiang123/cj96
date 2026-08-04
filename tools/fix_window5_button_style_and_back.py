#!/usr/bin/env python3
from __future__ import annotations

import shutil
import sys
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
UI = ROOT / "ui"
MAIN_FTU = UI / "main.ftu"

BLUE = (0, 126, 255, 255)
TEXT_BLUE = (0, 92, 190, 255)
FILL_TOP = (248, 253, 255, 255)
FILL_BOTTOM = (221, 241, 255, 255)
WHITE = (255, 255, 255, 255)
SCALE = 4

BUTTONS = {
    "Open_Valve.png": ("faucet_open", "开阀", 18),
    "Close_Valve.png": ("faucet_closed", "关阀", 18),
    "Sensor.png": ("sensor", "传感器", 16),
    "Sensor_Selected.png": ("sensor", "传感器", 16),
    "Solenoid_Valve.png": ("solenoid", "电磁阀", 16),
    "Solenoid_Valve_Selected.png": ("solenoid", "电磁阀", 16),
    "Source_Address.png": ("source", "源地址", 16),
    "Target_Address.png": ("target", "目标地址", 16),
    "Edit_Address.png": ("edit", "修改地址", 14),
    "Test_Address.png": ("test", "测试地址", 14),
    "Force_Edit.png": ("force", "强制修改", 14),
}

GLOBAL_SCREENSHOT_POSITION = {"left": 95, "top": 8, "width": 105, "height": 55}


def font(size: int) -> ImageFont.FreeTypeFont:
    candidates = [
        ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf",
        Path(r"C:\Windows\Fonts\msyh.ttc"),
        Path(r"C:\Windows\Fonts\simhei.ttf"),
    ]
    for path in candidates:
        if path.exists():
            return ImageFont.truetype(str(path), size)
    return ImageFont.load_default()


class SmoothDraw:
    """Draw using logical 120x60 coordinates onto a supersampled canvas."""

    def __init__(self, draw: ImageDraw.ImageDraw, scale: int) -> None:
        self.draw = draw
        self.scale = scale

    def _n(self, value: float | int) -> int:
        return int(round(float(value) * self.scale))

    def _xy(self, xy):
        if isinstance(xy, tuple) and len(xy) == 4 and not isinstance(xy[0], tuple):
            return tuple(self._n(v) for v in xy)
        return [(self._n(x), self._n(y)) for x, y in xy]

    def rounded_rectangle(self, xy, radius=0, fill=None, outline=None, width=1):
        self.draw.rounded_rectangle(
            self._xy(xy),
            radius=self._n(radius),
            fill=fill,
            outline=outline,
            width=max(1, self._n(width)),
        )

    def rectangle(self, xy, fill=None, outline=None, width=1):
        self.draw.rectangle(self._xy(xy), fill=fill, outline=outline, width=max(1, self._n(width)))

    def ellipse(self, xy, fill=None, outline=None, width=1):
        self.draw.ellipse(self._xy(xy), fill=fill, outline=outline, width=max(1, self._n(width)))

    def arc(self, xy, start, end, fill=None, width=1):
        self.draw.arc(self._xy(xy), start=start, end=end, fill=fill, width=max(1, self._n(width)))

    def line(self, xy, fill=None, width=1, joint=None):
        self.draw.line(self._xy(xy), fill=fill, width=max(1, self._n(width)), joint=joint)

    def polygon(self, xy, fill=None, outline=None):
        self.draw.polygon(self._xy(xy), fill=fill, outline=outline)

    def textbbox(self, xy, text, font=None):
        raw = self.draw.textbbox((self._n(xy[0]), self._n(xy[1])), text, font=font)
        return tuple(v / self.scale for v in raw)

    def text(self, xy, text, font=None, fill=None):
        self.draw.text((self._n(xy[0]), self._n(xy[1])), text, font=font, fill=fill)


def backup_files() -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup = ROOT / "Release" / f"before_window5_button_style_back_{stamp}"
    backup.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup / "main.ftu")
    for path in UI.glob("*.ftu"):
        shutil.copy2(path, backup / path.name)
    for name in BUTTONS:
        src = RESOURCES / name
        if src.exists():
            shutil.copy2(src, backup / name)
    return backup


def draw_gradient_panel(draw: ImageDraw.ImageDraw, image: Image.Image) -> None:
    scale = getattr(draw, "scale", 1)
    width, height = image.size
    logical_width = width // scale
    logical_height = height // scale
    mask = Image.new("L", image.size, 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.rounded_rectangle(
        (
            1 * scale,
            1 * scale,
            (logical_width - 2) * scale,
            (logical_height - 2) * scale,
        ),
        radius=7 * scale,
        fill=255,
    )
    gradient = Image.new("RGBA", image.size, WHITE)
    grad_pixels = gradient.load()
    for y in range(height):
        t = y / max(1, height - 1)
        color = tuple(
            int(FILL_TOP[i] * (1.0 - t) + FILL_BOTTOM[i] * t)
            for i in range(4)
        )
        for x in range(width):
            grad_pixels[x, y] = color
    image.alpha_composite(Image.composite(gradient, Image.new("RGBA", image.size, WHITE), mask))
    draw.rounded_rectangle((1, 1, logical_width - 2, logical_height - 2), radius=7, outline=BLUE, width=2)


def icon_box(draw: ImageDraw.ImageDraw) -> None:
    draw.rounded_rectangle((10, 11, 46, 47), radius=5, fill=(246, 252, 255, 255), outline=BLUE, width=2)


def line(draw: ImageDraw.ImageDraw, pts: list[tuple[int, int]], width: int = 3) -> None:
    draw.line(pts, fill=TEXT_BLUE, width=width, joint="curve")


def draw_faucet(draw: ImageDraw.ImageDraw, closed: bool) -> None:
    icon_box(draw)
    line(draw, [(19, 36), (19, 24), (29, 24), (29, 19), (39, 19)], 3)
    line(draw, [(18, 36), (28, 36), (39, 36)], 3)
    draw.rectangle((16, 31, 23, 42), outline=TEXT_BLUE, width=2)
    draw.arc((31, 24, 47, 43), start=260, end=70, fill=TEXT_BLUE, width=3)
    draw.line((34, 43, 47, 43), fill=TEXT_BLUE, width=3)
    draw.rounded_rectangle((26, 15, 36, 19), radius=2, outline=TEXT_BLUE, width=2)
    if closed:
        line(draw, [(28, 38), (41, 25)], 3)
    else:
        line(draw, [(32, 29), (45, 29)], 3)
        draw.polygon([(45, 29), (39, 25), (39, 33)], fill=TEXT_BLUE)


def draw_sensor(draw: ImageDraw.ImageDraw) -> None:
    icon_box(draw)
    draw.ellipse((25, 25, 33, 33), outline=TEXT_BLUE, width=3)
    for box in [(20, 20, 38, 38), (15, 15, 43, 43)]:
        draw.arc(box, 125, 235, fill=TEXT_BLUE, width=3)
        draw.arc(box, -55, 55, fill=TEXT_BLUE, width=3)
    draw.line((29, 29, 35, 29), fill=TEXT_BLUE, width=3)


def draw_solenoid(draw: ImageDraw.ImageDraw) -> None:
    icon_box(draw)
    draw.rounded_rectangle((20, 30, 39, 39), radius=2, outline=TEXT_BLUE, width=3)
    for x in [23, 28, 33]:
        draw.line((x, 31, x - 3, 38), fill=TEXT_BLUE, width=2)
    draw.line((15, 35, 20, 35), fill=TEXT_BLUE, width=3)
    draw.line((39, 35, 45, 35), fill=TEXT_BLUE, width=3)
    draw.rounded_rectangle((32, 20, 40, 29), radius=2, outline=TEXT_BLUE, width=2)
    draw.line((36, 20, 36, 16), fill=TEXT_BLUE, width=2)
    draw.line((40, 20, 45, 20), fill=TEXT_BLUE, width=2)


def draw_source(draw: ImageDraw.ImageDraw) -> None:
    icon_box(draw)
    draw.ellipse((26, 15, 34, 23), outline=TEXT_BLUE, width=3)
    draw.ellipse((22, 27, 30, 35), outline=TEXT_BLUE, width=3)
    draw.ellipse((34, 29, 42, 37), outline=TEXT_BLUE, width=3)
    line(draw, [(30, 23), (26, 28)], 2)
    line(draw, [(31, 23), (38, 31)], 2)
    line(draw, [(20, 39), (29, 35), (39, 39)], 3)
    line(draw, [(21, 21), (16, 17), (14, 24)], 2)
    line(draw, [(43, 20), (47, 16), (49, 24)], 2)


def draw_target(draw: ImageDraw.ImageDraw) -> None:
    icon_box(draw)
    draw.arc((17, 16, 40, 39), 195, 345, fill=TEXT_BLUE, width=3)
    draw.polygon([(39, 19), (46, 19), (43, 26)], fill=TEXT_BLUE)
    draw.arc((18, 18, 42, 42), 15, 165, fill=TEXT_BLUE, width=3)
    draw.polygon([(20, 39), (13, 39), (16, 32)], fill=TEXT_BLUE)
    draw.ellipse((27, 27, 32, 32), fill=TEXT_BLUE)


def draw_edit(draw: ImageDraw.ImageDraw) -> None:
    icon_box(draw)
    draw.rounded_rectangle((17, 21, 35, 38), radius=2, outline=TEXT_BLUE, width=3)
    line(draw, [(21, 28), (31, 28)], 2)
    line(draw, [(21, 34), (28, 34)], 2)
    line(draw, [(32, 36), (43, 25)], 3)
    draw.polygon([(42, 22), (46, 26), (43, 28), (39, 24)], fill=TEXT_BLUE)


def draw_test(draw: ImageDraw.ImageDraw) -> None:
    icon_box(draw)
    draw.rounded_rectangle((17, 20, 39, 39), radius=2, outline=TEXT_BLUE, width=3)
    line(draw, [(21, 26), (33, 26)], 2)
    line(draw, [(21, 32), (30, 32)], 2)
    line(draw, [(29, 34), (34, 38), (43, 27)], 3)


def draw_force(draw: ImageDraw.ImageDraw) -> None:
    icon_box(draw)
    draw.rounded_rectangle((15, 28, 29, 41), radius=2, outline=TEXT_BLUE, width=3)
    draw.arc((17, 17, 29, 32), 180, 360, fill=TEXT_BLUE, width=3)
    draw.ellipse((20, 33, 24, 37), fill=TEXT_BLUE)
    draw.rounded_rectangle((32, 28, 45, 40), radius=2, outline=TEXT_BLUE, width=2)
    line(draw, [(34, 33), (43, 24)], 3)
    line(draw, [(35, 37), (43, 37)], 2)


def draw_icon(draw: ImageDraw.ImageDraw, kind: str) -> None:
    if kind == "faucet_open":
        draw_faucet(draw, closed=False)
    elif kind == "faucet_closed":
        draw_faucet(draw, closed=True)
    elif kind == "sensor":
        draw_sensor(draw)
    elif kind == "solenoid":
        draw_solenoid(draw)
    elif kind == "source":
        draw_source(draw)
    elif kind == "target":
        draw_target(draw)
    elif kind == "edit":
        draw_edit(draw)
    elif kind == "test":
        draw_test(draw)
    elif kind == "force":
        draw_force(draw)
    else:
        raise ValueError(kind)


def render_button(kind: str, text: str, text_size: int, selected: bool = False) -> Image.Image:
    image = Image.new("RGBA", (120 * SCALE, 60 * SCALE), WHITE)
    draw = SmoothDraw(ImageDraw.Draw(image), SCALE)
    draw_gradient_panel(draw, image)
    if selected:
        draw.rounded_rectangle((2, 2, 117, 57), radius=7, outline=(0, 92, 190, 255), width=2)
    draw_icon(draw, kind)
    text_font = font(text_size * SCALE)
    bbox = draw.textbbox((0, 0), text, font=text_font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]
    x = 83 - text_w // 2
    y = 30 - text_h // 2 - 1
    draw.text((x, y), text, font=text_font, fill=TEXT_BLUE)
    return image.resize((120, 60), Image.Resampling.LANCZOS)


def render_buttons() -> None:
    for name, (kind, text, text_size) in BUTTONS.items():
        selected = name.endswith("_Selected.png")
        render_button(kind, text, text_size, selected).save(RESOURCES / name, optimize=True)


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def update_main_ftu() -> None:
    data, header, _ = decode_ftu(MAIN_FTU)
    for node in walk(data):
        if node.get("caption") == "GlobalScreenshotButton":
            node["position"] = dict(GLOBAL_SCREENSHOT_POSITION)
        if node.get("caption") in {
            "UartValueOnButton",
            "UartValueOffButton",
            "SenserRadioButton",
            "ValueRadioButton",
            "TextView12",
            "TextView17",
            "ChangeAdressOkButton",
            "TestAdressOkButton",
            "MustChangeAdressButton",
        }:
            node["text"] = ""
            node["position"]["width"] = 120
            node["position"]["height"] = 60
            node["iconPosition"] = {"left": 0, "top": 0, "width": 120, "height": 60}
            node.pop("textPosition", None)

    back_button = {
        "caption": "sys_back",
        "id": 100,
        "position": {"height": 70, "left": 0, "top": 0, "width": 87},
        "text": None,
        "picTab": {"pic0": "topset_back_36.png"},
        "colorTab": {"color0": 1477631},
        "iconPosition": {"height": 36, "left": 22, "top": 10, "width": 36},
        "textPosition": {"height": 50, "left": 0, "top": 0, "width": 80},
    }
    existing_back = next((node for node in walk(data) if node.get("caption") == "sys_back"), None)
    if existing_back is None:
        data["button__sys_back"] = back_button
    else:
        existing_back.clear()
        existing_back.update(back_button)

    MAIN_FTU.write_bytes(encode_ftu(data, header))


def update_all_screenshot_positions() -> list[str]:
    changed: list[str] = []
    for path in sorted(UI.glob("*.ftu")):
        data, header, _ = decode_ftu(path)
        file_changed = False
        for node in walk(data):
            if node.get("caption") == "GlobalScreenshotButton":
                if node.get("position") != GLOBAL_SCREENSHOT_POSITION:
                    node["position"] = dict(GLOBAL_SCREENSHOT_POSITION)
                    file_changed = True
        if file_changed:
            path.write_bytes(encode_ftu(data, header))
            changed.append(path.name)
    return changed


def main() -> None:
    backup = backup_files()
    render_buttons()
    update_main_ftu()
    changed = update_all_screenshot_positions()
    print(f"fixed Window5 button style, added main sys_back, moved screenshot buttons; changed={changed}; backup={backup}")


if __name__ == "__main__":
    main()
