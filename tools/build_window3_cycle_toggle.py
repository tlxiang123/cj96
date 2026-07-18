#!/usr/bin/env python3
"""Build dedicated antialiased 65px toggle states for CycleButton."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
RESOURCE_DIR = ROOT / "resources"
SCALE = 8
SIZE = 65

# Colors sampled from the currently used toggle resources.
OFF_COLOR = (83, 100, 117, 255)
ON_COLOR = (52, 199, 89, 255)
KNOB_COLOR = (255, 244, 226, 255)


def scaled_box(box: tuple[int, int, int, int]) -> tuple[int, int, int, int]:
    return tuple(value * SCALE for value in box)


def make_toggle(selected: bool) -> Image.Image:
    canvas = Image.new("RGBA", (SIZE * SCALE, SIZE * SCALE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(canvas)

    track_box = scaled_box((2, 17, 63, 48))
    draw.rounded_rectangle(
        track_box,
        radius=16 * SCALE,
        fill=ON_COLOR if selected else OFF_COLOR,
    )

    # The 27x27 knob shares the track's 32.5px vertical center. Its top and
    # bottom are exactly two pixels inside the 17..48 track bounds.
    knob_box = (33, 19, 60, 46) if selected else (5, 19, 32, 46)
    draw.ellipse(scaled_box(knob_box), fill=KNOB_COLOR)

    return canvas.resize((SIZE, SIZE), Image.Resampling.LANCZOS)


def verify(path: Path) -> None:
    image = Image.open(path).convert("RGBA")
    if image.size != (SIZE, SIZE):
        raise RuntimeError(f"wrong CycleButton toggle size: {image.size}")
    alpha_values = set(image.getchannel("A").getdata())
    if not any(0 < value < 255 for value in alpha_values):
        raise RuntimeError(f"missing antialiased edge pixels: {path.name}")


def main() -> None:
    outputs = (
        (False, "window3_cycle_toggle_off_65.png"),
        (True, "window3_cycle_toggle_on_65.png"),
    )
    for selected, filename in outputs:
        path = RESOURCE_DIR / filename
        make_toggle(selected).save(path, optimize=True)
        verify(path)
    print("built antialiased CycleButton toggle states")


if __name__ == "__main__":
    main()
