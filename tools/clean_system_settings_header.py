#!/usr/bin/env python3
"""Build the compact transparent settings header from the supplied artwork."""

from pathlib import Path

from PIL import Image

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "tools" / "assets" / "page1topset" / "system_settings_source.png"
OUTPUT = ROOT / "resources" / "topset_settings_header_clean_158x108.png"
UI_PATH = ROOT / "ui" / "page1topset.ftu"


def extract_foreground() -> Image.Image:
    source = Image.open(SOURCE).convert("RGB")
    # The supplied border lies outside this central gear-and-caption crop.
    source = source.crop((160, 24, 365, 298))
    output = Image.new("RGBA", source.size, (0, 0, 0, 0))
    source_pixels = source.load()
    output_pixels = output.load()

    for y in range(source.height):
        for x in range(source.width):
            red, green, blue = source_pixels[x, y]
            blue_strength = max(blue - red, blue - green)
            # The supplied screenshot has a faint blue panel. Keep only the
            # strongly blue gear/text pixels so that no tinted rectangle remains.
            alpha = max(0, min(255, round((blue_strength - 28) * 5.0)))
            if blue > 125 and alpha:
                output_pixels[x, y] = (red, green, blue, alpha)

    bounds = output.getchannel("A").getbbox()
    if bounds is None:
        raise RuntimeError("settings icon extraction produced an empty image")
    return output.crop(bounds)


def find_caption(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_caption(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_caption(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def main() -> None:
    foreground = extract_foreground()
    scale = min(82 / foreground.width, 98 / foreground.height)
    foreground = foreground.resize(
        (round(foreground.width * scale), round(foreground.height * scale)),
        Image.Resampling.LANCZOS,
    )

    output = Image.new("RGBA", (158, 108), (0, 0, 0, 0))
    output.alpha_composite(
        foreground,
        ((output.width - foreground.width) // 2, (output.height - foreground.height) // 2),
    )
    output.save(OUTPUT, optimize=True)

    data, header, _ = decode_ftu(UI_PATH)
    try:
        title = find_caption(data, "Button1")
    except KeyError:
        title = find_caption(data, "TextView7")
    title["backgroundPic"] = OUTPUT.name
    title["text"] = ""
    title["position"] = {"height": 108, "left": 433, "top": 2, "width": 158}
    UI_PATH.write_bytes(encode_ftu(data, header))

    print(f"saved {OUTPUT}")
    print(f"foreground={foreground.size}, alpha_bbox={output.getchannel('A').getbbox()}")


if __name__ == "__main__":
    main()
