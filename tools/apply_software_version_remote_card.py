#!/usr/bin/env python3
"""Turn the settings remote-upgrade entry into a software-version popup."""

from __future__ import annotations

import shutil
from pathlib import Path

from PIL import Image, ImageDraw

from ftu_style import decode_ftu, encode_ftu
from firmware_version import read_firmware_version


ROOT = Path(__file__).resolve().parents[1]
UI = ROOT / "ui" / "page1topset.ftu"
SOURCE = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-e6b942a5-067e-40ea-b868-ecefee9f5e3f.png")
CARD = ROOT / "resources" / "topset_remote_upgrade_113.png"


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def find(root: dict, caption: str) -> dict:
    for node in walk(root):
        if isinstance(node, dict) and node.get("caption") == caption:
            return node
    raise LookupError(caption)


def make_card() -> None:
    source = Image.open(SOURCE).convert("RGB")

    # Draw the same 113x113 white rounded card without borrowing another
    # card's icon artwork.
    result = Image.new("RGBA", (113, 113), (0, 0, 0, 0))
    ImageDraw.Draw(result).rounded_rectangle(
        (1, 1, 111, 111), radius=11, fill=(255, 255, 255, 255),
        outline=(180, 222, 246, 255), width=2,
    )

    def blue_artwork(crop_box: tuple[int, int, int, int]) -> Image.Image:
        artwork = source.crop(crop_box).convert("RGBA")
        pixels = artwork.load()
        for y in range(artwork.height):
            for x in range(artwork.width):
                r, g, b, _ = pixels[x, y]
                strength = min(255, max(0, (b - r - 28) * 5)) if b > g + 8 else 0
                pixels[x, y] = (r, g, b, strength) if strength else (r, g, b, 0)
        bbox = artwork.getchannel("A").getbbox()
        return artwork.crop(bbox) if bbox else artwork

    # Scale the document separately so its visual size matches the clock and
    # language icons. Keep the label at the same readable scale underneath.
    document = blue_artwork((185, 35, 365, 205))
    document.thumbnail((62, 64), Image.Resampling.LANCZOS)
    label = blue_artwork((130, 215, 420, 305))
    label.thumbnail((76, 24), Image.Resampling.LANCZOS)
    result.alpha_composite(document, ((113 - document.width) // 2, 9))
    result.alpha_composite(label, ((113 - label.width) // 2, 80))
    result.save(CARD, optimize=True)


def main() -> None:
    make_card()
    data, header, _ = decode_ftu(UI)
    frame = find(data, "TopSetContentFrameWindow")
    dialog = find(data, "update_wnd")
    button = find(data, "RemoteUpgradeButton")
    button["picTab"] = {"pic0": CARD.name, "pic1": CARD.name, "pic2": CARD.name}
    version_text = find(data, "RemoteUpgradeVersionText")
    version_text["text"] = "软件版本：" + read_firmware_version()

    for key, value in list(dialog.items()):
        if isinstance(value, dict) and value.get("caption") in {
            "RemoteUpgradeContentText", "RemoteUpgradeHintText",
            "RemoteUpgradeCancelButton", "RemoteUpgradeConfirmButton",
            "RemoteUpgradeProgressBar", "RemoteUpgradeProgressText",
        }:
            del dialog[key]
    dialog["modal"] = False

    backup = ROOT / "Release" / "page1topset_before_software_version.ftu"
    shutil.copy2(UI, backup)
    UI.write_bytes(encode_ftu(data, header))
    print(f"updated={UI}")
    print(f"card={CARD}")
    print(f"backup={backup}")


if __name__ == "__main__":
    main()
