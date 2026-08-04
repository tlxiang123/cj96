#!/usr/bin/env python3
"""Create the 158x100 system-settings header from the supplied icon card."""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path(r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-ea09d8e3-0074-4e1d-a5f7-7aa9d0421e94.png")
OUTPUT = ROOT / "resources" / "topset_system_settings_header_158x100.png"
FONT_CANDIDATES = (
    Path(r"C:\Windows\Fonts\msyh.ttc"),
    Path(r"C:\Windows\Fonts\simhei.ttf"),
)


def main() -> None:
    source = Image.open(SOURCE).convert("RGBA")
    # Remove the soft shadow below the card while keeping the blue border.
    source = source.crop((0, 0, source.width, min(320, source.height)))
    scale = 4
    image = source.resize((source.width * scale, source.height * scale), Image.Resampling.LANCZOS)
    draw = ImageDraw.Draw(image)
    draw.rectangle((48 * scale, 214 * scale, 438 * scale, 299 * scale), fill=(239, 248, 255, 255))

    font_path = next((path for path in FONT_CANDIDATES if path.exists()), None)
    if font_path is None:
        raise FileNotFoundError("No Chinese Windows font was found")
    font = ImageFont.truetype(str(font_path), 55 * scale)
    draw.text(
        (source.width * scale // 2, 255 * scale),
        "系统设置",
        font=font,
        fill=(0, 91, 184, 255),
        anchor="mm",
    )
    image.resize((158, 100), Image.Resampling.LANCZOS).save(OUTPUT, optimize=True)
    print(f"created {OUTPUT}")


if __name__ == "__main__":
    main()
