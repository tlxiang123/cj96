#!/usr/bin/env python3
from __future__ import annotations

import shutil
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"

WINDOW1_BG = (207, 227, 250, 255)
PANEL_TOP = (254, 255, 255, 255)
PANEL_BOTTOM = (239, 249, 255, 255)
BUTTON_OUTSIDE = (243, 250, 255, 255)
BUTTON_TOP = (253, 255, 255, 255)
BUTTON_BOTTOM = (239, 249, 255, 255)
BLUE = (0, 104, 205, 255)
LIGHT_BORDER = (0, 143, 255, 225)
PANEL_BORDER = (29, 151, 255, 230)

REFS = {
    "close_sensor": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-0c22c179-1b58-4ef3-b655-c230eac9b066.png"),
    "solenoid_source": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-30a3ff84-8ae6-4311-891f-e1b9383d920c.png"),
    "target_edit": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-46c8582f-3874-4739-94e8-9834e3b65fb4.png"),
    "test_force": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-a5d8ea5c-0dc2-4c34-b8c4-8dacc4b01cd4.png"),
    "open_old": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-22c952f4-65e9-4270-9b13-5c7825203e91.png"),
}

FULL_BUTTON_CROPS = {
    "Close_Valve.png": ("close_sensor", (70, 28, 526, 200)),
    "Sensor.png": ("close_sensor", (61, 272, 530, 479)),
    "Solenoid_Valve.png": ("solenoid_source", (82, 42, 566, 218)),
    "Source_Address.png": ("solenoid_source", (82, 293, 559, 494)),
    "Target_Address.png": ("target_edit", (86, 45, 542, 213)),
    "Edit_Address.png": ("target_edit", (84, 301, 543, 490)),
    "Test_Address.png": ("test_force", (94, 42, 554, 198)),
    "Force_Edit.png": ("test_force", (86, 290, 558, 475)),
}

ICON_CONTENT_CROPS = {
    "Close_Valve.png": (115, 50, 260, 158),
    "Sensor.png": (126, 326, 244, 414),
    "Sensor_Selected.png": (126, 326, 244, 414),
    "Solenoid_Valve.png": (126, 74, 260, 146),
    "Solenoid_Valve_Selected.png": (126, 74, 260, 146),
    "Source_Address.png": (146, 344, 266, 430),
    "Target_Address.png": (124, 64, 245, 171),
    "Edit_Address.png": (126, 340, 260, 424),
    "Test_Address.png": (125, 72, 266, 158),
    "Force_Edit.png": (126, 336, 282, 430),
}

OPEN_ICON_CROP = (116, 80, 252, 174)
OPEN_TEXT_CROP = (394, 144, 484, 202)

FONT_PATH = ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf"
CURRENT_OUTPUT = ""

LABELS = {
    "Open_Valve.png": "开阀",
    "Close_Valve.png": "关阀",
    "Sensor.png": "传感器",
    "Sensor_Selected.png": "传感器",
    "Solenoid_Valve.png": "电磁阀",
    "Solenoid_Valve_Selected.png": "电磁阀",
    "Source_Address.png": "源地址",
    "Target_Address.png": "目标地址",
    "Edit_Address.png": "修改地址",
    "Test_Address.png": "测试地址",
    "Force_Edit.png": "强制修改",
}

OUTPUTS = [
    "window5_background_1007x400.png",
    "window5_region1_frame_430x340.png",
    "window5_region2_frame_405x340.png",
    "Open_Valve.png",
    "Close_Valve.png",
    "Sensor.png",
    "Sensor_Selected.png",
    "Solenoid_Valve.png",
    "Solenoid_Valve_Selected.png",
    "Source_Address.png",
    "Target_Address.png",
    "Edit_Address.png",
    "Test_Address.png",
    "Force_Edit.png",
]


def backup_files() -> Path:
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup = ROOT / "Release" / f"before_window5_clean_reference_foregrounds_{stamp}"
    backup.mkdir(parents=True, exist_ok=False)
    for name in OUTPUTS:
        src = RESOURCES / name
        if src.exists():
            shutil.copy2(src, backup / name)
    return backup


def rounded_mask(size: tuple[int, int], radius: int, inset: int = 1) -> Image.Image:
    scale = 4
    mask = Image.new("L", (size[0] * scale, size[1] * scale), 0)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle(
        (inset * scale, inset * scale, (size[0] - 1 - inset) * scale, (size[1] - 1 - inset) * scale),
        radius=radius * scale,
        fill=255,
    )
    return mask.resize(size, Image.Resampling.LANCZOS)


def clean_button_base(selected: bool = False) -> Image.Image:
    width, height = 120, 60
    scale = 4
    image = Image.new("RGBA", (width * scale, height * scale), BUTTON_OUTSIDE)
    draw = ImageDraw.Draw(image)
    rect = (2 * scale, 2 * scale, (width - 3) * scale, (height - 3) * scale)
    radius = 9 * scale
    fill_image = vertical_gradient_rect(
        (width * scale, height * scale),
        (24, 155, 238, 255) if selected else BUTTON_TOP,
        (12, 137, 228, 255) if selected else BUTTON_BOTTOM,
    )
    fill_mask = Image.new("L", (width * scale, height * scale), 0)
    ImageDraw.Draw(fill_mask).rounded_rectangle(rect, radius=radius, fill=255)
    image.paste(fill_image, (0, 0), fill_mask)
    outline = (0, 119, 220, 255) if selected else LIGHT_BORDER
    draw.rounded_rectangle(rect, radius=radius, outline=outline, width=1 * scale)
    image = image.resize((width, height), Image.Resampling.LANCZOS)
    # Keep the whole 120x60 asset opaque. FlyThings' PNG alpha blending creates
    # jagged artifacts on transparent rounded corners, so corners are pre-filled
    # with the Window5 panel interior color instead of using alpha.
    image.putalpha(255)
    return image


def vertical_gradient_rect(size: tuple[int, int], top: tuple[int, int, int, int], bottom: tuple[int, int, int, int]) -> Image.Image:
    width, height = size
    image = Image.new("RGBA", size, top)
    pixels = image.load()
    for y in range(height):
        ratio = y / max(1, height - 1)
        row = tuple(int(top[i] * (1.0 - ratio) + bottom[i] * ratio) for i in range(4))
        for x in range(width):
            pixels[x, y] = row
    return image


def render_window1_style_panel(size: tuple[int, int]) -> Image.Image:
    width, height = size
    scale = 4
    image = Image.new("RGBA", (width * scale, height * scale), WINDOW1_BG)
    panel = vertical_gradient_rect((width * scale, height * scale), PANEL_TOP, PANEL_BOTTOM)
    mask = Image.new("L", (width * scale, height * scale), 0)
    mask_draw = ImageDraw.Draw(mask)
    rect = (1 * scale, 1 * scale, (width - 2) * scale, (height - 2) * scale)
    mask_draw.rounded_rectangle(rect, radius=13 * scale, fill=255)
    image.paste(panel, (0, 0), mask)
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(rect, radius=13 * scale, outline=PANEL_BORDER, width=2 * scale)
    return image.resize(size, Image.Resampling.LANCZOS)


def render_window5_background() -> None:
    Image.new("RGBA", (1007, 400), WINDOW1_BG).save(RESOURCES / "window5_background_1007x400.png", optimize=True)
    render_window1_style_panel((430, 340)).save(RESOURCES / "window5_region1_frame_430x340.png", optimize=True)
    render_window1_style_panel((405, 340)).save(RESOURCES / "window5_region2_frame_405x340.png", optimize=True)


def blue_foreground(crop: Image.Image, selected: bool = False) -> Image.Image:
    image = crop.convert("RGBA")
    r, g, b, _ = image.split()
    blue_score = ImageChops.lighter(ImageChops.subtract(b, r), ImageChops.subtract(b, g))
    mask = Image.eval(blue_score, lambda px: max(0, min(255, (px - 16) * 4)))
    mask = mask.filter(ImageFilter.GaussianBlur(0.75))
    color = (255, 255, 255, 255) if selected else BLUE
    foreground = Image.new("RGBA", image.size, color)
    foreground.putalpha(mask)
    return foreground


def remove_inner_box_and_text(image: Image.Image) -> Image.Image:
    alpha = image.getchannel("A")
    width, height = alpha.size
    pix = alpha.load()
    visited = bytearray(width * height)
    components: list[tuple[int, int, int, int, int]] = []

    for sy in range(height):
        for sx in range(width):
            idx = sy * width + sx
            if visited[idx] or pix[sx, sy] <= 12:
                visited[idx] = 1
                continue
            stack = [(sx, sy)]
            visited[idx] = 1
            min_x = max_x = sx
            min_y = max_y = sy
            count = 0
            while stack:
                x, y = stack.pop()
                count += 1
                if x < min_x:
                    min_x = x
                if x > max_x:
                    max_x = x
                if y < min_y:
                    min_y = y
                if y > max_y:
                    max_y = y
                for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                    if nx < 0 or nx >= width or ny < 0 or ny >= height:
                        continue
                    nidx = ny * width + nx
                    if visited[nidx] or pix[nx, ny] <= 12:
                        visited[nidx] = 1
                        continue
                    visited[nidx] = 1
                    stack.append((nx, ny))
            if count >= 6:
                components.append((count, min_x, min_y, max_x + 1, max_y + 1))

    if not components:
        return image

    # Anything clearly to the right is usually leftover reference text.
    largest = max(components, key=lambda item: item[0])
    keep_mask = Image.new("L", (width, height), 0)
    keep_draw = ImageDraw.Draw(keep_mask)
    for count, min_x, min_y, max_x, max_y in components:
        center_x = (min_x + max_x) / 2
        if center_x <= width * 0.62:
            keep_draw.rectangle((min_x, min_y, max_x, max_y), fill=255)

    result = image.copy()
    alpha = ImageChops.multiply(alpha, keep_mask)

    # Many reference icons contain a second, smaller rounded rectangle around the
    # pictogram. The user wants the same logic as the open-valve icon: one button
    # frame only, with the pictogram enlarged directly. Remove that inner frame.
    count, min_x, min_y, max_x, max_y = largest
    box_w = max_x - min_x
    box_h = max_y - min_y
    if box_w >= height * 0.35 and box_h >= height * 0.30:
        ap = alpha.load()
        thick = max(3, int(min(box_w, box_h) * 0.10))
        for y in range(max(0, min_y - 2), min(height, max_y + 2)):
            for x in range(max(0, min_x - 2), min(width, max_x + 2)):
                near_left = abs(x - min_x) <= thick
                near_right = abs(x - (max_x - 1)) <= thick
                near_top = abs(y - min_y) <= thick
                near_bottom = abs(y - (max_y - 1)) <= thick
                if near_left or near_right or near_top or near_bottom:
                    ap[x, y] = 0

    # Drop tiny specks left from the reference label or removed frame.
    cleaned = Image.new("L", (width, height), 0)
    cp = cleaned.load()
    ap = alpha.load()
    visited2 = bytearray(width * height)
    for sy in range(height):
        for sx in range(width):
            idx = sy * width + sx
            if visited2[idx] or ap[sx, sy] <= 12:
                visited2[idx] = 1
                continue
            stack = [(sx, sy)]
            visited2[idx] = 1
            pixels: list[tuple[int, int]] = []
            while stack:
                x, y = stack.pop()
                pixels.append((x, y))
                for nx, ny in ((x - 1, y), (x + 1, y), (x, y - 1), (x, y + 1)):
                    if nx < 0 or nx >= width or ny < 0 or ny >= height:
                        continue
                    nidx = ny * width + nx
                    if visited2[nidx] or ap[nx, ny] <= 12:
                        visited2[nidx] = 1
                        continue
                    visited2[nidx] = 1
                    stack.append((nx, ny))
            if len(pixels) >= 14:
                for x, y in pixels:
                    cp[x, y] = ap[x, y]

    result.putalpha(cleaned.filter(ImageFilter.GaussianBlur(0.25)))
    return result


def fit_icon_foreground(crop: Image.Image, selected: bool = False) -> Image.Image:
    # The caller passes the real pictogram area from the reference art. Scale
    # this directly, the same way the open-valve button is built, so the small
    # reference pictogram frame is never copied into the 120x60 button.
    fg_full = blue_foreground(crop.convert("RGBA"), selected=selected)
    alpha = fg_full.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        return Image.new("RGBA", (120, 60), (0, 0, 0, 0))

    pad_x = max(2, crop.width // 50)
    pad_y = max(2, crop.height // 50)
    box = (
        max(0, bbox[0] - pad_x),
        max(0, bbox[1] - pad_y),
        min(crop.width, bbox[2] + pad_x),
        min(crop.height, bbox[3] + pad_y),
    )
    icon = fg_full.crop(box)

    target_w, target_h = 48, 42
    if LABELS.get("_current") in {"源地址", "目标地址"}:
        target_w, target_h = 40, 42
    target_w, target_h = 50, 42
    if CURRENT_OUTPUT in {"Source_Address.png", "Target_Address.png"}:
        target_w, target_h = 46, 42
    ratio = min(target_w / icon.width, target_h / icon.height)
    new_size = (max(1, int(icon.width * ratio)), max(1, int(icon.height * ratio)))
    icon = icon.resize(new_size, Image.Resampling.LANCZOS)

    # One very light soften pass removes staircase edges without making the icon blurry.
    a = icon.getchannel("A").filter(ImageFilter.GaussianBlur(0.18))
    icon.putalpha(a)

    result = Image.new("RGBA", (120, 60), (0, 0, 0, 0))
    x = 8 + (50 - new_size[0]) // 2
    y = 9 + (42 - new_size[1]) // 2
    result.alpha_composite(icon, (x, y))
    return result


def draw_label(image: Image.Image, text: str, selected: bool = False) -> None:
    scale = 4
    canvas = Image.new("RGBA", (image.width * scale, image.height * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)
    max_width = 56 * scale
    font_size = 18 if len(text) <= 2 else 16 if len(text) <= 3 else 15
    while font_size >= 11:
        font = ImageFont.truetype(str(FONT_PATH), font_size * scale)
        bbox = draw.textbbox((0, 0), text, font=font)
        if bbox[2] - bbox[0] <= max_width:
            break
        font_size -= 1
    font = ImageFont.truetype(str(FONT_PATH), font_size * scale)
    bbox = draw.textbbox((0, 0), text, font=font)
    text_w = bbox[2] - bbox[0]
    text_h = bbox[3] - bbox[1]
    x = int((62 * scale) + (max_width - text_w) / 2)
    y = int((30 * scale) - text_h / 2 - bbox[1])
    color = (255, 255, 255, 255) if selected else BLUE
    draw.text((x, y), text, font=font, fill=color)
    image.alpha_composite(canvas.resize(image.size, Image.Resampling.LANCZOS))


def render_standard_button(
    source: Image.Image,
    box: tuple[int, int, int, int],
    output_name: str,
    selected: bool = False,
) -> Image.Image:
    global CURRENT_OUTPUT
    base = clean_button_base(selected=selected)
    CURRENT_OUTPUT = output_name
    LABELS["_current"] = LABELS[output_name]
    fg = fit_icon_foreground(source.crop(ICON_CONTENT_CROPS[output_name]), selected=selected)
    LABELS.pop("_current", None)
    CURRENT_OUTPUT = ""
    base.alpha_composite(fg)
    draw_label(base, LABELS[output_name], selected=selected)
    return base


def render_open_button(open_source: Image.Image, selected: bool = False) -> Image.Image:
    base = clean_button_base(selected=selected)
    icon = blue_foreground(open_source.crop(OPEN_ICON_CROP), selected=selected).resize((48, 35), Image.Resampling.LANCZOS)
    base.alpha_composite(icon, (10, 12))
    draw_label(base, LABELS["Open_Valve.png"], selected=selected)
    return base


def render_buttons() -> None:
    missing = [str(path) for path in REFS.values() if not path.exists()]
    if missing:
        raise FileNotFoundError("missing reference image(s): " + ", ".join(missing))

    sources = {name: Image.open(path).convert("RGBA") for name, path in REFS.items()}
    render_open_button(sources["open_old"]).save(RESOURCES / "Open_Valve.png", optimize=True)
    for output_name, (source_name, box) in FULL_BUTTON_CROPS.items():
        render_standard_button(sources[source_name], box, output_name).save(RESOURCES / output_name, optimize=True)

    render_standard_button(sources["close_sensor"], FULL_BUTTON_CROPS["Sensor.png"][1], "Sensor_Selected.png", selected=True).save(
        RESOURCES / "Sensor_Selected.png", optimize=True
    )
    render_standard_button(
        sources["solenoid_source"], FULL_BUTTON_CROPS["Solenoid_Valve.png"][1], "Solenoid_Valve_Selected.png", selected=True
    ).save(
        RESOURCES / "Solenoid_Valve_Selected.png", optimize=True
    )


def main() -> None:
    backup = backup_files()
    render_window5_background()
    render_buttons()
    print(f"applied clean Window5 reference foreground buttons; backup={backup}")


if __name__ == "__main__":
    main()
