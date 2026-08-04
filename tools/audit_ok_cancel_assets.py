#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
from PIL import Image
import json
import sys


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"


def image_info(path: Path) -> dict:
    im = Image.open(path).convert("RGBA")
    pixels = list(im.getdata())
    alpha_values = {px[3] for px in pixels}
    corners = [
        im.getpixel((0, 0)),
        im.getpixel((im.width - 1, 0)),
        im.getpixel((0, im.height - 1)),
        im.getpixel((im.width - 1, im.height - 1)),
    ]
    edge = []
    for x in range(im.width):
        edge.append(im.getpixel((x, 0)))
        edge.append(im.getpixel((x, im.height - 1)))
    for y in range(im.height):
        edge.append(im.getpixel((0, y)))
        edge.append(im.getpixel((im.width - 1, y)))
    opaque_edge = [px for px in edge if px[3] == 255]
    edge_colors = {}
    for px in opaque_edge:
        edge_colors[px] = edge_colors.get(px, 0) + 1
    top_edge = sorted(edge_colors.items(), key=lambda item: item[1], reverse=True)[:8]
    return {
        "name": path.name,
        "size": im.size,
        "mode": Image.open(path).mode,
        "has_alpha": len(alpha_values) > 1 or next(iter(alpha_values)) < 255,
        "alpha_values_sample": sorted(alpha_values)[:10],
        "corners": corners,
        "top_edge_colors": top_edge,
    }


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    names = [
        "w2_ok_120x60.png",
        "w2_cancel_120x60.png",
        "w2_group_bind_cancel_120x60.png",
        "w2_group_bind_confirm_120x60.png",
        "w2_group_rename_confirm_120x60.png",
        "cycle_ok_confirm_120x60.png",
    ]
    found = []
    for name in names:
        path = RESOURCE_DIR / name
        if path.is_file():
            found.append(image_info(path))
    print(json.dumps(found, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
