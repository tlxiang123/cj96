from pathlib import Path

from PIL import Image


RESOURCE_DIR = Path('resources')
OUTPUT_DIR = Path('backups/flat_dark_icon_preview')
NAMES = ('bgr_btn1', 'bgr_btn2', 'bgr_btn3', 'bgr_btn_log')
TEST_NAME = 'bgr_btn4'
FRAME_NAME = 'bgr_btn_log'
TEST_SOURCE = Path('C:/Users/Administrator/AppData/Local/Temp/codex-clipboard-2e96840a-c0df-42fc-8bd0-15d83d24ed3f.png')
TARGET_COLOR = (0, 87, 183)
SCALE = 6
GAP = 28
BACKGROUND = (246, 248, 251)


def flatten_color(image):
    flattened = image.copy()
    pixels = flattened.load()
    for y in range(flattened.height):
        for x in range(flattened.width):
            red, green, blue, alpha = pixels[x, y]
            if alpha == 0 or (red > 240 and green > 240 and blue > 240):
                continue
            pixels[x, y] = (*TARGET_COLOR, alpha)
    return flattened


def rebuild_test():
    frame = flatten_color(Image.open(RESOURCE_DIR / (FRAME_NAME + '.png')).convert('RGBA'))
    source = Image.open(TEST_SOURCE).convert('RGBA')
    icon = source.crop((16, 48, 145, 175))
    available_top = 11
    available_bottom = frame.height - 10
    available_left = 9
    available_right = frame.width - 9
    available_width = available_right - available_left
    available_height = available_bottom - available_top
    scale = min(available_width / icon.width, available_height / icon.height)
    icon = flatten_color(icon.resize((round(icon.width * scale), round(icon.height * scale)), Image.Resampling.LANCZOS))
    result = Image.new('RGBA', frame.size, (0, 0, 0, 0))
    result.alpha_composite(frame)
    result.alpha_composite(icon, (available_left + (available_width - icon.width) // 2, available_top + (available_height - icon.height) // 2))
    return result


def save_comparison(images):
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    resized = []
    board_height = 0
    for name, image in images:
        image.save(OUTPUT_DIR / (name + '.png'))
        large = image.resize((image.width * SCALE, image.height * SCALE), Image.Resampling.NEAREST)
        large.save(OUTPUT_DIR / (name + '_large.png'))
        resized.append((name, large))
        board_height = max(board_height, large.height)

    board_width = sum(image.width for _, image in resized) + GAP * (len(resized) - 1)
    board = Image.new('RGB', (board_width, board_height), BACKGROUND)
    cursor_x = 0
    for _, image in resized:
        board.paste(image, (cursor_x, (board_height - image.height) // 2), image)
        cursor_x += image.width + GAP
    board.save(OUTPUT_DIR / 'all_flat_dark_icons.png')


def main():
    images = [(name, flatten_color(Image.open(RESOURCE_DIR / (name + '.png')).convert('RGBA'))) for name in NAMES]
    images.append((TEST_NAME, rebuild_test()))
    save_comparison(images)


if __name__ == '__main__':
    main()
