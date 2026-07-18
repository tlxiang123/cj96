#!/usr/bin/env python3
"""Create the Window8 irrigation ring panel and its runtime text controls."""

from __future__ import annotations

import shutil
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "main.ftu"
OUTPUT_PATH = ROOT / "resources" / "window8_panel.png"
BACKUP_PATH = ROOT / "ui" / "main.ftu.before-window8-dashboard-20260716.bak"

PANEL_SIZE = (581, 374)
RING_CENTER = (220, 187)
RING_OUTER_RADIUS = 170
RING_INNER_RADIUS = 100


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


def _vertical_gradient(
    size: tuple[int, int],
    top_color: tuple[int, int, int, int],
    bottom_color: tuple[int, int, int, int],
) -> Image.Image:
    image = Image.new("RGBA", size, top_color)
    draw = ImageDraw.Draw(image)
    for y in range(size[1]):
        ratio = y / max(1, size[1] - 1)
        color = tuple(
            round(start + (end - start) * ratio)
            for start, end in zip(top_color, bottom_color)
        )
        draw.line((0, y, size[0] - 1, y), fill=color)
    return image


def _draw_ring_segment(
    canvas: Image.Image,
    outer_box: tuple[int, int, int, int],
    inner_box: tuple[int, int, int, int],
    start_angle: int,
    end_angle: int,
    top_color: tuple[int, int, int, int],
    bottom_color: tuple[int, int, int, int],
) -> None:
    mask = Image.new("L", canvas.size, 0)
    draw = ImageDraw.Draw(mask)
    draw.pieslice(outer_box, start=start_angle, end=end_angle, fill=255)
    draw.ellipse(inner_box, fill=0)
    canvas.alpha_composite(Image.composite(
        _vertical_gradient(canvas.size, top_color, bottom_color),
        Image.new("RGBA", canvas.size),
        mask,
    ))


def create_panel() -> None:
    scale = 4
    width, height = PANEL_SIZE
    large_size = (width * scale, height * scale)
    # Keep pixels outside the rounded panel transparent so the parent Window1
    # background shows through without a white fringe at the corners.
    panel = Image.new("RGBA", large_size, (0, 0, 0, 0))
    panel_fill = _vertical_gradient(
        large_size,
        (255, 255, 255, 255),
        (238, 247, 253, 255),
    )
    panel_mask = Image.new("L", large_size, 0)
    mask_draw = ImageDraw.Draw(panel_mask)
    mask_draw.rounded_rectangle(
        (2 * scale, 2 * scale, (width - 2) * scale - 1, (height - 2) * scale - 1),
        radius=20 * scale,
        fill=255,
    )
    panel.alpha_composite(Image.composite(
        panel_fill, Image.new("RGBA", large_size), panel_mask
    ))

    cx, cy = RING_CENTER[0] * scale, RING_CENTER[1] * scale
    outer_radius = RING_OUTER_RADIUS * scale
    inner_radius = RING_INNER_RADIUS * scale
    outer_box = (
        cx - outer_radius,
        cy - outer_radius,
        cx + outer_radius,
        cy + outer_radius,
    )
    inner_box = (
        cx - inner_radius,
        cy - inner_radius,
        cx + inner_radius,
        cy + inner_radius,
    )

    shadow = Image.new("RGBA", large_size, (0, 0, 0, 0))
    shadow_draw = ImageDraw.Draw(shadow)
    shadow_draw.ellipse(
        tuple(value + (3 * scale if index % 2 else 0) for index, value in enumerate(outer_box)),
        fill=(0, 88, 150, 45),
    )
    panel.alpha_composite(shadow.filter(ImageFilter.GaussianBlur(7 * scale)))

    _draw_ring_segment(
        panel, outer_box, inner_box, -89, 30,
        (31, 222, 105, 255), (7, 180, 72, 255),
    )
    _draw_ring_segment(
        panel, outer_box, inner_box, 31, 149,
        (15, 190, 238, 255), (0, 137, 207, 255),
    )
    _draw_ring_segment(
        panel, outer_box, inner_box, 150, 269,
        (255, 220, 54, 255), (255, 181, 0, 255),
    )

    ring_draw = ImageDraw.Draw(panel)
    ring_draw.ellipse(outer_box, outline=(255, 255, 255, 255), width=4 * scale)
    ring_draw.ellipse(inner_box, outline=(255, 255, 255, 255), width=4 * scale)
    ring_draw.rounded_rectangle(
        (2 * scale, 2 * scale, (width - 2) * scale - 1, (height - 2) * scale - 1),
        radius=20 * scale,
        outline=(22, 139, 255, 255),
        width=2 * scale,
    )

    output = panel.resize(PANEL_SIZE, Image.Resampling.LANCZOS)
    output.save(OUTPUT_PATH, format="PNG", optimize=True)


def _text_view(
    caption: str,
    control_id: int,
    text: str,
    position: tuple[int, int, int, int],
    font_size: int,
    color: int,
    bold: bool = True,
) -> dict:
    left, top, width, height = position
    return {
        "alignment": 37,
        "bold": bold,
        "caption": caption,
        "colorTab": {"color0": color},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": font_size,
        "id": control_id,
        "text": text,
        "touchable": False,
        "position": {"height": height, "left": left, "top": top, "width": width},
    }


def main() -> None:
    create_panel()
    layout, header, _ = decode_ftu(UI_PATH)
    window8 = find_caption(layout, "Window8")
    shutil.copy2(UI_PATH, BACKUP_PATH)

    window8["position"] = {"height": 374, "left": 400, "top": 16, "width": 581}
    window8["backgroundPic"] = "window8_panel.png"
    window8.pop("backgroundColor", None)

    captions = {
        "Window8CompletedGroupText",
        "Window8RunningGroupText",
        "Window8WaitingGroupText",
        "Window8StatusLine1Text",
        "Window8StatusLine2Text",
        "Window8StatusLine3Text",
        "Window8LegendRunningText",
        "Window8LegendWaitingText",
        "Window8LegendCompletedText",
    }
    for key in list(window8):
        value = window8[key]
        if isinstance(value, dict) and value.get("caption") in captions:
            del window8[key]

    controls = (
        _text_view("Window8CompletedGroupText", 50055, "1", (314, 95, 50, 46), 32, 0xFFFFFF),
        _text_view("Window8RunningGroupText", 50056, "2", (195, 301, 50, 46), 32, 0xFFFFFF),
        _text_view("Window8WaitingGroupText", 50057, "3", (76, 95, 50, 46), 32, 0xFFFFFF),
        _text_view("Window8StatusLine1Text", 50058, "", (120, 126, 200, 34), 20, 0x0072B5),
        _text_view("Window8StatusLine2Text", 50059, "", (120, 170, 200, 34), 18, 0x0072B5),
        _text_view("Window8StatusLine3Text", 50060, "", (120, 214, 200, 34), 18, 0x0072B5),
        _text_view("Window8LegendRunningText", 50061, "● 灌溉中", (398, 116, 170, 38), 23, 0x008FD5, False),
        _text_view("Window8LegendWaitingText", 50062, "● 等待", (398, 168, 170, 38), 23, 0xE5A700, False),
        _text_view("Window8LegendCompletedText", 50063, "● 完成", (398, 220, 170, 38), 23, 0x0AAF61, False),
    )
    for index, control in enumerate(controls, 204):
        window8[f"textview__{index}"] = control

    UI_PATH.write_bytes(encode_ftu(layout, header))
    verified, _, _ = decode_ftu(UI_PATH)
    verified_window = find_caption(verified, "Window8")
    for caption in captions:
        find_caption(verified_window, caption)
    if verified_window.get("backgroundPic") != "window8_panel.png":
        shutil.copy2(BACKUP_PATH, UI_PATH)
        raise RuntimeError("Window8 dashboard verification failed")
    print("Window8 dashboard applied")


if __name__ == "__main__":
    main()
