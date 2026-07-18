#!/usr/bin/env python3
"""Flatten the Window8 panel to RGB for the target renderer."""

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
path = ROOT / "resources" / "window8_panel.png"
image = Image.open(path).convert("RGBA")
background = Image.new("RGBA", image.size, (255, 255, 255, 255))
background.alpha_composite(image)
background.convert("RGB").save(path, format="PNG", optimize=True)
print("flattened Window8 panel to RGB", path)
