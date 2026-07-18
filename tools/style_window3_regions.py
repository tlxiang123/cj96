#!/usr/bin/env python3
"""Group Window3 controls into seven rounded visual regions."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "main.ftu"
RESOURCE_DIR = ROOT / "resources"
SCALE = 4

REGIONS = (
    ("Window3Region1Window", 110021, (0, 0, 265, 69), (
        "Button15", "CycleButton",
    )),
    ("Window3Region2Window", 110022, (263, 0, 424, 69), (
        "PervProgButton", "ShowProgEditText", "NextProgButton",
    )),
    ("Window3Region3Window", 110023, (687, 0, 285, 69), (
        "OnOffProgTextButton", "OnOffProgButton",
    )),
    ("Window3Region4Window", 110024, (0, 73, 985, 76), (
        "Button14", "Button10", "StartTimeHour1EditText", "TextView8",
        "StartTimeMin1EditText", "Button11", "StartTimeHour2EditText",
        "TextView9", "StartTimeMin2EditText", "Button12",
        "StartTimeHour3EditText", "TextView10", "StartTimeMin3EditText",
        "Button13", "StartTimeHour4EditText", "TextView11",
        "StartTimeMin4EditText",
    )),
    ("Window3Region5Window", 110025, (0, 151, 528, 158), (
        "WeekModeButton", "EverDayButton", "SundayButton", "MondayButton",
        "TuesdayButton", "WednesdayButton", "ThursdayButton", "FridayButton",
        "SaturdayButton",
    )),
    ("Window3Region6Window", 110026, (0, 309, 500, 70), (
        "IntervalModeButton", "IntervalDayEditText", "Button26", "Button27",
        "AfterDayEditText",
    )),
    ("Window3Region7Window", 110027, (270, 390, 465, 230), (
        "RunTimeListView",
    )),
)

SCROLL_CAPTION = "Window3ScrollWindow"
SCROLL_ID = 32001
CONTENT_CAPTION = "Window3ScrollContentWindow"
CONTENT_ID = 110028
CONTENT_HEIGHT = 630


def find_control(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def make_panel(width: int, height: int, output: Path) -> None:
    size = (width * SCALE, height * SCALE)
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    gradient = Image.new("RGBA", size)
    pixels = gradient.load()
    top = (255, 253, 248, 248)
    bottom = (244, 250, 255, 248)
    for y in range(size[1]):
        ratio = y / max(1, size[1] - 1)
        color = tuple(round(top[i] + (bottom[i] - top[i]) * ratio) for i in range(4))
        for x in range(size[0]):
            pixels[x, y] = color

    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (1 * SCALE, 1 * SCALE, size[0] - SCALE - 1, size[1] - SCALE - 1),
        radius=9 * SCALE,
        fill=255,
    )
    image.paste(gradient, (0, 0), mask)
    ImageDraw.Draw(image).rounded_rectangle(
        (1 * SCALE, 1 * SCALE, size[0] - SCALE - 1, size[1] - SCALE - 1),
        radius=9 * SCALE,
        outline=(73, 174, 238, 255),
        width=2 * SCALE,
    )
    image.resize((width, height), Image.Resampling.LANCZOS).save(output, optimize=True)


def patch_ftu() -> None:
    data, header, _ = decode_ftu(FTU_PATH)
    window = find_control(data, "Window3")

    region_captions = {caption for caption, _, _, _ in REGIONS}
    target_captions = {
        child_caption
        for _, _, _, captions in REGIONS
        for child_caption in captions
    }
    caption_to_region = {
        caption: region_index
        for region_index, (_, _, _, captions) in enumerate(REGIONS)
        for caption in captions
    }
    grouped: list[list[tuple[str, dict]]] = [[] for _ in REGIONS]
    relative_positions: dict[str, tuple[int, int]] = {}

    for region_caption in region_captions:
        try:
            existing_region = find_control(window, region_caption)
        except KeyError:
            continue
        for child_key, child in existing_region.items():
            child_caption = child.get("caption") if isinstance(child, dict) else None
            if child_caption in target_captions:
                grouped[caption_to_region[child_caption]].append((child_key, child))
                relative_positions[child_caption] = (
                    child["position"]["left"], child["position"]["top"]
                )

    retained: dict = {}
    insertion_key = None

    for key, value in window.items():
        caption = value.get("caption") if isinstance(value, dict) else None
        if caption == SCROLL_CAPTION:
            continue
        if caption in region_captions:
            continue
        if caption in caption_to_region:
            if not relative_positions:
                grouped[caption_to_region[caption]].append((key, value))
            continue
        if insertion_key is None and isinstance(value, dict) and caption in {
            "SetRunTimeWindow", "W3TipWindow"
        }:
            insertion_key = key
        retained[key] = value

    missing = [
        caption for _, _, _, captions in REGIONS for caption in captions
        if not any(item[1].get("caption") == caption for items in grouped for item in items)
    ]
    if missing:
        raise RuntimeError(f"Window3 region controls missing: {missing}")

    region_nodes = []
    for index, ((caption, control_id, box, _), controls) in enumerate(zip(REGIONS, grouped), 1):
        left, top, width, height = box
        node = {
            "beepEnable": True,
            "caption": caption,
            "id": control_id,
            "visible": True,
            "backgroundPic": f"window3_region_{index}.png",
            "position": {"height": height, "left": left, "top": top, "width": width},
        }
        for key, control in controls:
            child_caption = control["caption"]
            if child_caption in relative_positions:
                child_left, child_top = relative_positions[child_caption]
                control["position"]["left"] = child_left
                control["position"]["top"] = child_top
            else:
                if child_caption == "OnOffProgTextButton":
                    control["position"]["left"] = 695
                control["position"]["left"] -= left
                control["position"]["top"] -= top
            node[key] = control
        region_nodes.append((f"window__{104 + index}", node))

    content = {
        "backgroundColor": 0xCFE3FA,
        "beepEnable": True,
        "caption": CONTENT_CAPTION,
        "id": CONTENT_ID,
        "position": {"height": CONTENT_HEIGHT, "left": 0, "top": 0, "width": 1007},
    }
    content.update(region_nodes)
    scroll = {
        "caption": SCROLL_CAPTION,
        "dragMaxDis": 200,
        "edgeEffect": 1,
        "id": SCROLL_ID,
        "orientation": 1,
        "position": {"height": 400, "left": 0, "top": 0, "width": 1007},
        "window__104": content,
    }

    rebuilt = {}
    inserted = False
    for key, value in retained.items():
        if not inserted and key == insertion_key:
            rebuilt["scrollwindow__103"] = scroll
            inserted = True
        rebuilt[key] = value
    if not inserted:
        rebuilt["scrollwindow__103"] = scroll
    window.clear()
    window.update(rebuilt)

    FTU_PATH.write_bytes(encode_ftu(data, header))


def verify() -> None:
    data, _, _ = decode_ftu(FTU_PATH)
    window = find_control(data, "Window3")
    scroll = find_control(window, SCROLL_CAPTION)
    content = find_control(scroll, CONTENT_CAPTION)
    if scroll.get("orientation") != 1 or content["position"]["height"] != CONTENT_HEIGHT:
        raise RuntimeError("invalid Window3 vertical scroll container")
    for index, (caption, control_id, _, captions) in enumerate(REGIONS, 1):
        region = find_control(content, caption)
        if region.get("id") != control_id or region.get("backgroundPic") != f"window3_region_{index}.png":
            raise RuntimeError(f"invalid Window3 region: {caption}")
        for child_caption in captions:
            find_control(region, child_caption)
    region7 = find_control(content, "Window3Region7Window")
    if region7["position"]["top"] != 390:
        raise RuntimeError("Window3Region7Window is not at the bottom")


def main() -> None:
    for index, (_, _, (_, _, width, height), _) in enumerate(REGIONS, 1):
        make_panel(width, height, RESOURCE_DIR / f"window3_region_{index}.png")
    patch_ftu()
    verify()
    print("grouped Window3 controls into 7 regions")


if __name__ == "__main__":
    main()
