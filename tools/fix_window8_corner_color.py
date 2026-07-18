#!/usr/bin/env python3
"""Match Window8's outside rounded corners to Window1's background."""

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
path = ROOT / "resources" / "window8_panel.png"
source = Image.open(path).convert("RGB")
background = Image.new("RGB", source.size, (207, 227, 250))
mask = Image.new("L", source.size, 0)
ImageDraw.Draw(mask).rounded_rectangle(
    (2, 2, source.width - 3, source.height - 3), radius=20, fill=255
)
background.paste(source, (0, 0), mask)
background.save(path, format="PNG", optimize=True)
print("matched Window8 outside corners to Window1 background")
