#!/usr/bin/env python3
"""Add the local valve-operation log page and its fifth bottom navigation button."""

from __future__ import annotations

import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_FILE = ROOT / "ui" / "main.ftu"
SOURCE_ICON = Path(
    r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-44c43ef0-119f-4dfe-9d02-7bba16393b4f.png"
)

LOG_BUTTON_ID = 20220
LOG_WINDOW_ID = 110093
LOG_TIME_IDS = tuple(range(50300, 50306))
LOG_WEEK_IDS = tuple(range(50306, 50312))
LOG_MODE_IDS = tuple(range(50312, 50318))
LOG_ACTION_IDS = tuple(range(50318, 50324))
LOG_DETAIL_IDS = tuple(range(50324, 50330))
BUTTON_LEFTS = (222, 342, 462, 582, 702)


def color_to_int(rgb: tuple[int, int, int]) -> int:
    return (rgb[0] << 16) | (rgb[1] << 8) | rgb[2]


LOG_WINDOW_BG = "log_window_bg_1007x400.png"
LOG_TEXT_COLOR = color_to_int((0, 91, 187))
LOG_WEEK_COLOR = color_to_int((255, 107, 0))
LOG_ACTION_ON_COLOR = color_to_int((0, 200, 83))
LOG_ACTION_OFF_COLOR = color_to_int((149, 165, 166))


def make_log_window_background() -> None:
    outer = (207, 227, 250, 255)
    border = (0, 146, 255, 255)
    fill_top = (250, 254, 255, 255)
    fill_bottom = (238, 248, 255, 255)
    width, height = 1007, 400
    panel = (28, 22, 979, 376)
    radius = 17
    scale = 4

    for directory in (ROOT / "ui", ROOT / "resources"):
        image = Image.new("RGBA", (width * scale, height * scale), outer)
        draw = ImageDraw.Draw(image)
        left, top, right, bottom = (value * scale for value in panel)
        panel_w = right - left
        panel_h = bottom - top

        fill = Image.new("RGBA", (panel_w, panel_h), (0, 0, 0, 0))
        fill_draw = ImageDraw.Draw(fill)
        for y in range(panel_h):
            t = y / max(1, panel_h - 1)
            color = tuple(
                int(fill_top[i] * (1 - t) + fill_bottom[i] * t)
                for i in range(4)
            )
            fill_draw.line([(0, y), (panel_w, y)], fill=color)

        mask = Image.new("L", (panel_w, panel_h), 0)
        mask_draw = ImageDraw.Draw(mask)
        mask_draw.rounded_rectangle(
            (0, 0, panel_w - 1, panel_h - 1),
            radius=radius * scale,
            fill=255,
        )
        image.paste(fill, (left, top), mask)
        draw.rounded_rectangle(
            (left, top, right - 1, bottom - 1),
            radius=radius * scale,
            outline=border,
            width=2 * scale,
        )
        image.resize((width, height), Image.Resampling.LANCZOS).save(
            directory / LOG_WINDOW_BG
        )


def make_log_button_assets() -> None:
    if not SOURCE_ICON.exists():
        raise FileNotFoundError(f"log icon not found: {SOURCE_ICON}")

    source = Image.open(SOURCE_ICON).convert("RGBA")
    source_rgb = source.convert("RGB")
    white = Image.new("RGB", source_rgb.size, (255, 255, 255))
    content_box = ImageChops.difference(source_rgb, white).getbbox()
    if content_box is None:
        raise ValueError(f"log icon is empty: {SOURCE_ICON}")

    # The supplied screenshot contains a large white canvas around the button.
    # Crop that canvas first so the generated asset fills the same 100x70 slot
    # as the four existing navigation buttons.
    image = source.crop(content_box).resize((100, 70), Image.Resampling.LANCZOS)

    # The first four navigation buttons use a darker icon/text color while
    # selected. Keep the log button background and anti-aliased edges, but
    # apply the same pressed-state color to its centered icon and caption.
    selected = image.copy()
    pixels = selected.load()
    for y in range(6, 63):
        for x in range(10, 90):
            red, green, blue, alpha = pixels[x, y]
            blue_score = blue - red
            green_score = green - red
            ink_alpha = max(0.0, min(1.0, (blue_score - 45.0) / 130.0))
            ink_alpha *= max(0.0, min(1.0, (green_score - 12.0) / 70.0))
            if ink_alpha <= 0.0:
                continue
            target = (0, 67, 158)
            pixels[x, y] = tuple(
                int(round(channel * ink_alpha + original * (1.0 - ink_alpha)))
                for channel, original in zip(target, (red, green, blue))
            ) + (alpha,)

    for directory in (ROOT / "ui", ROOT / "resources"):
        image.save(directory / "bgr_btn_log.png")
        selected.save(directory / "bgr_btn_log_ch.png")
    make_log_window_background()


def text_view(
    caption: str,
    control_id: int,
    text: str,
    top: int,
    left: int,
    width: int,
    font_size: int,
    color: int = LOG_TEXT_COLOR,
    alignment: int = 33,
) -> dict:
    return {
        "alignment": alignment,
        "caption": caption,
        "colorTab": {"color0": color},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": font_size,
        "id": control_id,
        "text": text,
        "touchable": False,
        "position": {"height": 36, "left": left, "top": top, "width": width},
    }


def add_log_controls(data: dict) -> bool:
    changed = False

    for index, left in enumerate(BUTTON_LEFTS[:4], start=1):
        button = data[f"button__{index}"]
        if button["position"]["left"] != left:
            button["position"]["left"] = left
            changed = True

    log_button = next(
        (value for value in data.values()
         if isinstance(value, dict) and value.get("caption") == "LogButton"),
        None,
    )
    if log_button is None:
        data["button__324"] = {
            "alignment": 37,
            "caption": "LogButton",
            "colorTab": {"color0": 16777215},
            "family": "Alibaba-PuHuiTi-Regular",
            "iconPosition": {"height": 70, "left": 0, "top": 0, "width": 100},
            "id": LOG_BUTTON_ID,
            "picTab": {"pic0": "bgr_btn_log.png", "pic2": "bgr_btn_log_ch.png"},
            "position": {"height": 82, "left": BUTTON_LEFTS[4], "top": 518, "width": 99},
        }
        changed = True
    else:
        wanted_button = {
            "id": LOG_BUTTON_ID,
            "picTab": {"pic0": "bgr_btn_log.png", "pic2": "bgr_btn_log_ch.png"},
            "iconPosition": {"height": 70, "left": 0, "top": 0, "width": 100},
            "position": {"height": 82, "left": BUTTON_LEFTS[4], "top": 518, "width": 99},
        }
        for key, value in wanted_button.items():
            if log_button.get(key) != value:
                log_button[key] = value
                changed = True

    if not any(
        isinstance(value, dict) and value.get("caption") == "LogWindow"
        for value in data.values()
    ):
        log_window = {
            "backgroundColor": color_to_int((207, 227, 250)),
            "backgroundPic": LOG_WINDOW_BG,
            "beepEnable": True,
            "caption": "LogWindow",
            "id": LOG_WINDOW_ID,
            "visible": False,
            "position": {"height": 400, "left": 8, "top": 93, "width": 1007},
            "textview__1": {
                "alignment": 37,
                "bold": True,
                "caption": "LogTitleText",
                "colorTab": {"color0": 23483},
                "family": "Alibaba-PuHuiTi-Regular",
                "fontSize": 30,
                "id": 50299,
                "text": "灌溉日志",
                "touchable": False,
                "position": {"height": 46, "left": 300, "top": 42, "width": 400},
            },
        }
        for index, control_id in enumerate(LOG_LINE_IDS):
            text = "暂无电磁阀操作日志" if index == 0 else ""
            log_window[f"textview__{index + 2}"] = text_view(
                f"LogLine{index + 1}", control_id, text, 102 + index * 48, 23
            )
        data["window__325"] = log_window
        changed = True
    else:
        log_window = next(
            value for value in data.values()
            if isinstance(value, dict) and value.get("caption") == "LogWindow"
        )
        wanted_window = {
            "backgroundColor": color_to_int((207, 227, 250)),
            "backgroundPic": LOG_WINDOW_BG,
            "position": {"height": 400, "left": 8, "top": 93, "width": 1007},
        }
        for key, value in wanted_window.items():
            if log_window.get(key) != value:
                log_window[key] = value
                changed = True

        title = next(
            (value for value in log_window.values()
             if isinstance(value, dict) and value.get("caption") == "LogTitleText"),
            None,
        )
        if title is not None:
            wanted_title = {
                "position": {"height": 46, "left": 300, "top": 42, "width": 400},
                "fontSize": 30,
                "alignment": 37,
                "colorTab": {"color0": 23483},
            }
            for key, value in wanted_title.items():
                if title.get(key) != value:
                    title[key] = value
                    changed = True

        for index in range(1, 7):
            line = next(
                (value for value in log_window.values()
                 if isinstance(value, dict) and value.get("caption") == f"LogLine{index}"),
                None,
            )
            if line is not None:
                wanted_line = {
                    "position": {
                        "height": 36,
                        "left": 70,
                        "top": 102 + (index - 1) * 48,
                        "width": 860,
                    },
                    "fontSize": 23,
                    "alignment": 33,
                    "colorTab": {"color0": 23483},
                }
                for key, value in wanted_line.items():
                    if line.get(key) != value:
                        line[key] = value
                        changed = True

    return changed


def main() -> None:
    make_log_button_assets()
    data, header, _ = decode_ftu(UI_FILE)
    changed = add_log_controls(data)
    if changed:
        UI_FILE.write_bytes(encode_ftu(data, header))

    decoded, _, _ = decode_ftu(UI_FILE)
    captions = {
        value.get("caption")
        for value in decoded.values()
        if isinstance(value, dict)
    }
    if not {"LogButton", "LogWindow"}.issubset(captions):
        raise RuntimeError("log controls were not written to main.ftu")
    print("local log page UI updated")


if __name__ == "__main__":
    main()
