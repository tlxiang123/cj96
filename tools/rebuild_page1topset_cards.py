#!/usr/bin/env python3
"""Rebuild page1topset as a framed six-card settings grid."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
SOURCE_DIR = ROOT / "tools" / "assets" / "page1topset"
FTU_PATH = ROOT / "ui" / "page1topset.ftu"
SCALE = 4

PANEL_SIZE = (984, 460)
PANEL_POSITION = {"height": 460, "left": 20, "top": 120, "width": 984}
CARD_SIZE = 113

# Crops contain exactly one supplied square icon, including its own border.
CARD_SPECS = {
    "OpenWifiButton": ("network.png", (16, 10, 260, 254), "topset_wifi_113.png"),
    "OpenNetButton": ("network.png", (16, 273, 260, 517), "topset_ethernet_113.png"),
    "Open4GButton": ("mobile_time.png", (52, 11, 297, 256), "topset_4g_113.png"),
    "SetSysTimeBtn": ("mobile_time.png", (52, 274, 297, 519), "topset_time_113.png"),
    "LanBtn": ("language_display.png", (72, 23, 317, 268), "topset_language_113.png"),
    "DisplayBtn": ("language_display.png", (72, 285, 317, 530), "topset_display_113.png"),
}

CARD_POSITIONS = {
    "OpenWifiButton": (98, 55),
    "OpenNetButton": (323, 55),
    "Open4GButton": (548, 55),
    "SetSysTimeBtn": (773, 55),
    "LanBtn": (98, 250),
    "DisplayBtn": (323, 250),
}


def build_panel() -> Image.Image:
    width, height = PANEL_SIZE
    left = PANEL_POSITION["left"]
    top = PANEL_POSITION["top"]
    panel = Image.open(RESOURCE_DIR / "background.png").convert("RGBA").crop(
        (left, top, left + width, top + height)
    )

    # Draw only the frame as an antialiased overlay. The untouched crop below
    # keeps the root-page gradient and horizontal guide line pixel-aligned.
    overlay = Image.new(
        "RGBA", (width * SCALE, height * SCALE), (0, 0, 0, 0)
    )
    inset = 2 * SCALE
    box = (inset, inset, overlay.width - inset - 1, overlay.height - inset - 1)
    ImageDraw.Draw(overlay).rounded_rectangle(
        box, radius=14 * SCALE, outline=(142, 211, 255, 255), width=2 * SCALE
    )
    overlay = overlay.resize(PANEL_SIZE, Image.Resampling.LANCZOS)
    panel.alpha_composite(overlay)
    return panel


def build_card(source_name: str, crop: tuple[int, int, int, int]) -> Image.Image:
    source = Image.open(SOURCE_DIR / source_name).convert("RGB")
    card = source.crop(crop)
    if card.width != card.height:
        raise RuntimeError(f"non-square crop for {source_name}: {card.size}")
    card = card.resize((CARD_SIZE, CARD_SIZE), Image.Resampling.LANCZOS).convert("RGBA")

    # Remove the screenshot's rectangular corner background. The rounded
    # card remains opaque while its four outer corners reveal the panel below.
    mask = Image.new("L", (CARD_SIZE * SCALE, CARD_SIZE * SCALE), 0)
    inset = 2 * SCALE
    ImageDraw.Draw(mask).rounded_rectangle(
        (inset, inset, mask.width - inset - 1, mask.height - inset - 1),
        radius=15 * SCALE,
        fill=255,
    )
    card.putalpha(mask.resize((CARD_SIZE, CARD_SIZE), Image.Resampling.LANCZOS))
    return card


def build_back_icon() -> Image.Image:
    source = Image.open(SOURCE_DIR / "back.png").convert("RGB")
    extracted = Image.new("RGBA", source.size, (0, 0, 0, 0))
    source_pixels = source.load()
    output_pixels = extracted.load()
    for y in range(source.height):
        for x in range(source.width):
            red, green, blue = source_pixels[x, y]
            # Remove the white/light-blue screenshot background while keeping
            # the supplied antialiased blue arrow edge.
            alpha = max(0, min(255, round((blue - red - 20) * 2.2)))
            if alpha:
                output_pixels[x, y] = (red, green, blue, alpha)

    bounds = extracted.getbbox()
    if bounds is None:
        raise RuntimeError("back arrow extraction produced an empty image")
    arrow = extracted.crop(bounds)
    arrow.thumbnail((32, 32), Image.Resampling.LANCZOS)
    output = Image.new("RGBA", (36, 36), (0, 0, 0, 0))
    output.alpha_composite(
        arrow, ((36 - arrow.width) // 2, (36 - arrow.height) // 2)
    )
    return output


def build_content_frame() -> Image.Image:
    width, height = PANEL_SIZE
    large = Image.new(
        "RGBA", (width * SCALE, height * SCALE), (0, 0, 0, 0)
    )
    inset = 2 * SCALE
    ImageDraw.Draw(large).rounded_rectangle(
        (inset, inset, large.width - inset - 1, large.height - inset - 1),
        radius=12 * SCALE,
        outline=(73, 174, 238, 255),
        width=2 * SCALE,
    )
    return large.resize(PANEL_SIZE, Image.Resampling.LANCZOS)


def find_control(node: object, caption: str) -> tuple[dict, str, dict] | None:
    if not isinstance(node, dict):
        return None
    for key, value in node.items():
        if not isinstance(value, dict):
            continue
        if value.get("caption") == caption:
            return node, key, value
        found = find_control(value, caption)
        if found is not None:
            return found
    return None


def main() -> None:
    for _, (source_name, crop, output_name) in CARD_SPECS.items():
        build_card(source_name, crop).save(RESOURCE_DIR / output_name, optimize=True)
    back_name = "topset_back_36.png"
    build_back_icon().save(RESOURCE_DIR / back_name, optimize=True)
    frame_name = "topset_content_frame_984x460.png"
    build_content_frame().save(RESOURCE_DIR / frame_name, optimize=True)

    data, header, _ = decode_ftu(FTU_PATH)
    back_found = find_control(data, "sys_back")
    if back_found is None:
        raise RuntimeError("missing sys_back control")
    back = back_found[2]
    back["picTab"] = {"pic0": back_name}
    back["iconPosition"] = {
        "height": 36,
        "left": 54,
        "top": 18,
        "width": 36,
    }
    cards: dict[str, dict] = {}
    for caption in CARD_SPECS:
        found = find_control(data, caption)
        if found is None:
            raise RuntimeError(f"missing control: {caption}")
        parent, key, control = found
        cards[caption] = control
        del parent[key]

    for caption in ("Button2", "Button3"):
        found = find_control(data, caption)
        if found is not None:
            parent, key, _ = found
            del parent[key]

    header_found = find_control(data, "nettopwind")
    if header_found is None:
        raise RuntimeError("missing nettopwind header")
    header_parent, header_key, header_window = header_found
    del header_parent[header_key]
    header_window["backgroundPic"] = "nettopwind.png"

    old_content = find_control(data, "TopSetContentWindow")
    if old_content is not None:
        old_parent, old_key, _ = old_content
        del old_parent[old_key]

    old_page = find_control(data, "TopSetPageWindow")
    if old_page is not None:
        old_parent, old_key, _ = old_page
        del old_parent[old_key]

    content_window = {
        "backgroundColor": 16777215,
        "beepEnable": True,
        "caption": "TopSetContentWindow",
        "id": 110002,
        "position": dict(PANEL_POSITION),
        "window__0": {
            "backgroundPic": frame_name,
            "beepEnable": False,
            "caption": "TopSetContentFrameWindow",
            "id": 110004,
            "position": {"height": 460, "left": 0, "top": 0, "width": 984},
            "touchable": False,
        },
    }
    for index, caption in enumerate(CARD_SPECS, start=1):
        card = cards[caption]
        left, top = CARD_POSITIONS[caption]
        output_name = CARD_SPECS[caption][2]
        card.pop("text", None)
        card["position"] = {
            "height": CARD_SIZE,
            "left": left,
            "top": top,
            "width": CARD_SIZE,
        }
        card["iconPosition"] = {
            "height": CARD_SIZE,
            "left": 0,
            "top": 0,
            "width": CARD_SIZE,
        }
        card["picTab"] = {"pic0": output_name}
        content_window[f"button__{index}"] = card

    page_window = {
        "backgroundColor": 16777215,
        "beepEnable": True,
        "caption": "TopSetPageWindow",
        "id": 110003,
        "position": {"height": 600, "left": 0, "top": 0, "width": 1024},
        "window__1": header_window,
        "window__2": content_window,
    }
    data["window__1"] = page_window
    FTU_PATH.write_bytes(encode_ftu(data, header))

    decoded, _, _ = decode_ftu(FTU_PATH)
    verified = find_control(decoded, "TopSetContentWindow")
    if verified is None:
        raise RuntimeError("content window round-trip verification failed")
    for caption in CARD_SPECS:
        found = find_control(decoded, caption)
        if found is None or found[2]["position"]["width"] != CARD_SIZE:
            raise RuntimeError(f"card verification failed: {caption}")
    verified_page = find_control(decoded, "TopSetPageWindow")
    if verified_page is None or verified_page[2]["position"] != {
        "height": 600, "left": 0, "top": 0, "width": 1024
    }:
        raise RuntimeError("full-screen page window verification failed")
    if verified[2].get("backgroundPic"):
        raise RuntimeError("TopSetContentWindow must use an editable color")
    print("rebuilt page1topset: white full-screen parent and editable content color")


if __name__ == "__main__":
    main()
