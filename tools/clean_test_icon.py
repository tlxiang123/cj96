from pathlib import Path

from PIL import Image

SOURCE = Path('C:/Users/Administrator/AppData/Local/Temp/codex-clipboard-2e96840a-c0df-42fc-8bd0-15d83d24ed3f.png')
OUTPUT = Path('backups/flat_dark_icon_preview/bgr_btn4_clean.png')
COMPARE = Path('backups/flat_dark_icon_preview/bgr_btn4_clean_large.png')
TARGET = (0, 87, 183)

image = Image.open(SOURCE).convert('RGB')
image = image.crop((8, 26, 149, 164))
pixels = image.load()

for y in range(image.height):
    for x in range(image.width):
        red, green, blue = pixels[x, y]
        strength = blue - red
        if strength < 18:
            pixels[x, y] = (255, 255, 255)
        elif strength > 35:
            ratio = min(1.0, (strength - 35) / 80.0)
            pixels[x, y] = tuple(round(channel + (target - channel) * ratio) for channel, target in zip((red, green, blue), TARGET))

image = image.resize((70, 59), Image.Resampling.LANCZOS)

for y in range(image.height):
    for x in range(image.width):
        red, green, blue = pixels[x, y] if False else image.getpixel((x, y))
        if red > 245 and green > 245 and blue > 245:
            image.putpixel((x, y), (255, 255, 255, 255))
        else:
            image.putpixel((x, y), (*TARGET, 255))

image.save(OUTPUT)
image.resize((420, 354), Image.Resampling.NEAREST).save(COMPARE)
print(OUTPUT)