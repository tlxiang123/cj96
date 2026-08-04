#!/usr/bin/env python3
from __future__ import annotations

import shutil
import sys
from datetime import datetime
from pathlib import Path

from PIL import Image, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ftu_style import decode_ftu, encode_ftu  # noqa: E402


ROOT = Path(__file__).resolve().parents[1]
RESOURCES = ROOT / "resources"
MAIN_FTU = ROOT / "ui" / "main.ftu"

SOURCE_IMAGES = {
    "valves": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-22c952f4-65e9-4270-9b13-5c7825203e91.png"),
    "decoder": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-db069984-b201-46d1-a022-d2320d906862.png"),
    "addresses": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-331972a9-c82b-4081-bef4-53b84e83500b.png"),
    "edit": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-ca6c9553-70f3-403f-b7a9-578d04088273.png"),
    "force": Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-d00d0055-1d06-4e24-8b52-04482e6afb8e.png"),
}

# crop boxes are the rounded button panels from the user's reference screenshots.
CROPS = {
    "Open_Valve.png": ("valves", (55, 50, 515, 232)),
    "Close_Valve.png": ("valves", (72, 321, 522, 493)),
    "Sensor.png": ("decoder", (72, 40, 542, 249)),
    "Sensor_Selected.png": ("decoder", (72, 40, 542, 249)),
    "Solenoid_Valve.png": ("decoder", (75, 317, 545, 501)),
    "Solenoid_Valve_Selected.png": ("decoder", (75, 317, 545, 501)),
    "Source_Address.png": ("addresses", (75, 35, 545, 233)),
    "Target_Address.png": ("addresses", (84, 322, 540, 485)),
    "Edit_Address.png": ("edit", (83, 42, 540, 233)),
    "Test_Address.png": ("edit", (88, 330, 540, 483)),
    "Force_Edit.png": ("force", (82, 40, 546, 226)),
}

WINDOW5_TEXT_CONTROLS = {
    "UartValueOnButton",
    "UartValueOffButton",
    "SenserRadioButton",
    "ValueRadioButton",
    "TextView12",
    "TextView17",
    "ChangeAdressOkButton",
    "TestAdressOkButton",
    "MustChangeAdressButton",
}


def fit_to_120x60(crop: Image.Image) -> Image.Image:
    crop = crop.convert("RGBA")
    # Keep the user's panel proportions while filling the 120x60 target.
    # A small sharpen after downscale preserves the Chinese text strokes.
    image = crop.resize((120, 60), Image.Resampling.LANCZOS)
    image = image.filter(ImageFilter.UnsharpMask(radius=0.7, percent=85, threshold=2))
    return image


def find_caption(node: object, caption: str) -> dict | None:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            found = find_caption(value, caption)
            if found is not None:
                return found
    elif isinstance(node, (list, tuple)):
        for value in node:
            found = find_caption(value, caption)
            if found is not None:
                return found
    return None


def walk_dicts(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk_dicts(value)
    elif isinstance(node, (list, tuple)):
        for value in node:
            yield from walk_dicts(value)


def apply_ftu_text_cleanup() -> None:
    data, header, _offset = decode_ftu(MAIN_FTU)
    changed = []
    for node in walk_dicts(data):
        if node.get("caption") in WINDOW5_TEXT_CONTROLS:
            node["text"] = ""
            node.pop("textPosition", None)
            node["iconPosition"] = {"left": 0, "top": 0, "width": 120, "height": 60}
            node["position"]["width"] = 120
            node["position"]["height"] = 60
            changed.append(node["caption"])
    missing = sorted(WINDOW5_TEXT_CONTROLS - set(changed))
    if missing:
        raise RuntimeError(f"missing Window5 controls: {missing}")
    MAIN_FTU.write_bytes(encode_ftu(data, header))
    decoded, _header2, _offset2 = decode_ftu(MAIN_FTU)
    for caption in WINDOW5_TEXT_CONTROLS:
        node = find_caption(decoded, caption)
        if not node or node.get("text") != "" or node.get("position", {}).get("width") != 120 or node.get("position", {}).get("height") != 60:
            raise RuntimeError(f"FTU verify failed for {caption}: {node}")


def main() -> None:
    for name, path in SOURCE_IMAGES.items():
        if not path.exists():
            raise FileNotFoundError(f"{name}: {path}")

    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    backup = ROOT / "Release" / f"before_window5_reference_120x60_{stamp}"
    backup.mkdir(parents=True, exist_ok=False)
    shutil.copy2(MAIN_FTU, backup / "main.ftu")
    for resource_name in CROPS:
        src = RESOURCES / resource_name
        if src.exists():
            shutil.copy2(src, backup / resource_name)

    for output_name, (source_key, box) in CROPS.items():
        source = Image.open(SOURCE_IMAGES[source_key])
        image = fit_to_120x60(source.crop(box))
        image.save(RESOURCES / output_name, optimize=True)

    apply_ftu_text_cleanup()

    print(f"applied Window5 reference 120x60 buttons; backup={backup}")


if __name__ == "__main__":
    main()
