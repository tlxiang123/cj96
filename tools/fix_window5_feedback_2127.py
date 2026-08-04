#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys
import time

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, RESOURCE_DIR, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
BACKUP_DIR = ROOT / "Release" / f"before_window5_feedback_2127_{time.strftime('%Y%m%d_%H%M%S')}"

BLUE = 23483
INPUT_VISUAL = "debug_edit_inner_99x71.png"
SMALL_OUTLINE = "debug_outline_82x58.png"


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


def walk(node: object) -> list[dict]:
    out: list[dict] = []
    if isinstance(node, dict):
        if "caption" in node:
            out.append(node)
        for value in node.values():
            out.extend(walk(value))
    elif isinstance(node, list):
        for value in node:
            out.extend(walk(value))
    return out


def generate_input_visual() -> None:
    source = RESOURCE_DIR / SMALL_OUTLINE
    target = RESOURCE_DIR / INPUT_VISUAL
    if not source.is_file():
        raise FileNotFoundError(source)
    canvas = Image.new("RGBA", (99, 71), (0, 0, 0, 0))
    with Image.open(source) as image:
        small = image.convert("RGBA")
    canvas.paste(small, ((99 - small.width) // 2, (71 - small.height) // 2), small)
    canvas.save(target)


def set_address_input(node: dict, left: int, top: int) -> None:
    node["position"] = {"left": left, "top": top, "width": 99, "height": 71}
    node["backgroundPic"] = INPUT_VISUAL
    node.pop("bgColorTab", None)
    node["colorTab"] = {"color0": BLUE, "color1": BLUE, "color2": BLUE}
    node["fontSize"] = 32
    node["textAlign"] = "center"
    node["align"] = "center"
    node["gravity"] = "center"


def apply() -> None:
    path = UI_DIR / "main.ftu"
    data, header, _ = decode_ftu(path)
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found")

    window5["backgroundPic"] = "bgr_mid.png"

    # Make address input visual smaller while preserving 99x71 clickable area.
    set_address_input(find_caption(window5, "ValveAddressEditText"), 178, 112)
    set_address_input(find_caption(window5, "SrouceAddressEditText"), 613, 108)
    set_address_input(find_caption(window5, "DestAdressEditText"), 840, 108)

    # Decoder type title: remove the box, keep blue text only.
    decoder = find_caption(window5, "Button40")
    if decoder is not None:
        decoder.pop("picTab", None)
        decoder.pop("backgroundPic", None)
        decoder.pop("bgColorTab", None)
        decoder.pop("iconPosition", None)
        decoder["colorTab"] = {"color0": BLUE, "color1": BLUE, "color2": BLUE}
        decoder["fontSize"] = 24
        decoder["bold"] = True
        decoder["position"] = {"left": 40, "top": 199, "width": 368, "height": 45}

    # Move radio buttons up slightly with title so the left area remains balanced.
    group = find_caption(window5, "RadioGroup1")
    if group is not None:
        group["position"] = {"left": 40, "top": 258, "width": 395, "height": 79}

    # Move force button up to avoid bottom overlap.
    force = find_caption(window5, "MustChangeAdressButton")
    if force is not None:
        force["position"] = {"left": 693, "top": 282, "width": 148, "height": 71}
        force["iconPosition"] = {"left": 0, "top": 0, "width": 148, "height": 71}

    # Slightly center source/target labels over the smaller visuals.
    source_label = find_caption(window5, "TextView12")
    if source_label is not None:
        source_label["position"] = {"left": 608, "top": 58, "width": 109, "height": 47}
        source_label["colorTab"] = {"color0": 0}
    dest_label = find_caption(window5, "TextView17")
    if dest_label is not None:
        dest_label["position"] = {"left": 835, "top": 58, "width": 109, "height": 47}
        dest_label["colorTab"] = {"color0": 0}

    path.write_bytes(encode_ftu(data, header))
    decoded, _, _ = decode_ftu(path)
    if decoded != data:
        raise RuntimeError("round-trip mismatch")


def verify() -> None:
    data, _, _ = decode_ftu(UI_DIR / "main.ftu")
    window5 = find_caption(data, "Window5")
    if window5 is None:
        raise RuntimeError("Window5 not found")
    if window5.get("backgroundPic") != "bgr_mid.png":
        raise RuntimeError("Window5 backgroundPic is not bgr_mid.png")
    decoder = find_caption(window5, "Button40")
    if decoder is not None and (decoder.get("picTab") or decoder.get("backgroundPic") or decoder.get("bgColorTab")):
        raise RuntimeError("Button40 still has a frame/background")
    for caption in ("ValveAddressEditText", "SrouceAddressEditText", "DestAdressEditText"):
        node = find_caption(window5, caption)
        if node is None:
            raise RuntimeError(f"{caption} missing")
        if node.get("backgroundPic") != INPUT_VISUAL:
            raise RuntimeError(f"{caption} backgroundPic={node.get('backgroundPic')}")
        if node.get("colorTab", {}).get("color0") != BLUE:
            raise RuntimeError(f"{caption} color is not blue")
        pos = node.get("position") or {}
        if int(pos.get("width", 0)) != 99 or int(pos.get("height", 0)) != 71:
            raise RuntimeError(f"{caption} clickable position size changed: {pos}")
    force = find_caption(window5, "MustChangeAdressButton")
    if force and int(force.get("position", {}).get("top", 999)) > 290:
        raise RuntimeError("force button is still too low")


def backup() -> None:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    (BACKUP_DIR / "resources").mkdir(parents=True, exist_ok=True)
    shutil.copy2(UI_DIR / "main.ftu", BACKUP_DIR / "ui" / "main.ftu")
    for name in (INPUT_VISUAL, SMALL_OUTLINE, "bgr_mid.png"):
        source = RESOURCE_DIR / name
        if source.is_file():
            shutil.copy2(source, BACKUP_DIR / "resources" / name)


def main() -> None:
    backup()
    generate_input_visual()
    apply()
    verify()
    print(f"backup={BACKUP_DIR}")
    print("updated Window5 feedback: blue background, smaller input visual, blue address text, no decoder frame, force button raised")


if __name__ == "__main__":
    main()
