#!/usr/bin/env python3
"""Build Window3's number artwork from Window2's image-backed address digits."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
BLUE = (22, 117, 185, 255)


def source_digit_cards() -> dict[str, Image.Image]:
    source = Image.open(RESOURCE_DIR / "w2_set_address_value_020.png").convert("RGBA")
    # Window2's address image has three equally sized, individually framed
    # glyph cards. Reusing those cards keeps the customer-provided numeral look.
    cards = {}
    for digit, left in (("0", 8), ("2", 39)):
        cards[digit] = source.crop((left, 13, left + 28, 56))

    for digit in "13456789":
        value = int(digit)
        image = Image.open(
            RESOURCE_DIR / f"w2_set_address_value_{value:03d}.png"
        ).convert("RGBA")
        cards[digit] = image.crop((69, 13, 97, 56))
    return cards


def number_image(value: int, cards: dict[str, Image.Image]) -> Image.Image:
    output = Image.new("RGBA", (58, 43), (0, 0, 0, 0))
    text = f"{value:02d}"
    output.alpha_composite(cards[text[0]], (1, 0))
    output.alpha_composite(cards[text[1]], (29, 0))
    return output


def time_image(hour: int, minute: int, cards: dict[str, Image.Image]) -> Image.Image:
    output = Image.new("RGBA", (126, 43), (0, 0, 0, 0))
    output.alpha_composite(number_image(hour, cards), (0, 0))
    output.alpha_composite(number_image(minute, cards), (68, 0))
    draw = ImageDraw.Draw(output)
    draw.ellipse((62, 13, 66, 17), fill=BLUE)
    draw.ellipse((62, 27, 66, 31), fill=BLUE)
    return output


def empty_image() -> Image.Image:
    output = Image.new("RGBA", (58, 43), (0, 0, 0, 0))
    draw = ImageDraw.Draw(output)
    draw.rounded_rectangle((1, 1, 26, 41), radius=5, outline=BLUE, width=2)
    draw.rounded_rectangle((31, 1, 56, 41), radius=5, outline=BLUE, width=2)
    draw.line((8, 21, 19, 21), fill=BLUE, width=3)
    draw.line((38, 21, 49, 21), fill=BLUE, width=3)
    return output


def empty_time_image() -> Image.Image:
    output = Image.new("RGBA", (126, 43), (0, 0, 0, 0))
    output.alpha_composite(empty_image(), (0, 0))
    output.alpha_composite(empty_image(), (68, 0))
    draw = ImageDraw.Draw(output)
    draw.ellipse((62, 13, 66, 17), fill=BLUE)
    draw.ellipse((62, 27, 66, 31), fill=BLUE)
    return output


def picker_panel() -> Image.Image:
    scale = 4
    size = (360, 238)
    large = Image.new("RGBA", (size[0] * scale, size[1] * scale), (0, 0, 0, 0))
    draw = ImageDraw.Draw(large)
    draw.rectangle((0, 0, large.width - 1, large.height - 1), fill=(255, 253, 248, 255))
    line = (225, 230, 234, 255)
    for y in (52, 104, 156, 157):
        draw.line((0, y * scale, large.width - 1, y * scale), fill=line, width=scale)
    return large.resize(size, Image.Resampling.LANCZOS)


def main() -> None:
    cards = source_digit_cards()
    for value in range(100):
        number_image(value, cards).save(
            RESOURCE_DIR / f"window3_number_{value:02d}.png", optimize=True
        )
    for hour in range(25):
        for minute in range(60):
            time_image(hour, minute, cards).save(
                RESOURCE_DIR / f"window3_time_{hour:02d}_{minute:02d}.png",
                optimize=True,
            )
    empty_image().save(RESOURCE_DIR / "window3_number_empty.png", optimize=True)
    empty_time_image().save(RESOURCE_DIR / "window3_time_empty.png", optimize=True)
    picker_panel().save(RESOURCE_DIR / "window3_value_picker_panel_360x238.png", optimize=True)
    print("built Window3 number and time images from Window2 address digit artwork")


if __name__ == "__main__":
    main()
