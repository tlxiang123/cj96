from pathlib import Path
from PIL import Image

RESOURCE_DIR = Path(
"resources"
)
OUTPUT_DIR = Path("backups")
NAMES = ("bgr_btn1", "bgr_btn2", "bgr_btn3", "bgr_btn4", "bgr_btn_log")
NORMAL_BLEND = 0.45
SCALE = 4
GAP = 32
BACKGROUND = (247, 249, 252)

def resize(image):
    return image.resize((image.width * SCALE, image.height * SCALE), Image.Resampling.NEAREST)

def main():
    cards = []
    board_height = 0
    for name in NAMES:
        normal = Image.open(RESOURCE_DIR / (name + ".png")).convert("RGBA")
        current_pressed = Image.open(RESOURCE_DIR / (name + "_ch.png")).convert("RGBA")
        medium_pressed = Image.blend(current_pressed, normal, NORMAL_BLEND)
        medium_pressed.save(RESOURCE_DIR / (name + "_ch.png"))
        normal_large = resize(normal)
        pressed_large = resize(medium_pressed)
        cards.append((normal_large, pressed_large))
        board_height = max(board_height, normal_large.height, pressed_large.height)

    board_width = sum(normal.width + pressed.width for normal, pressed in cards)
    board_width += GAP * (len(cards) * 2 - 1)
    board = Image.new("RGB", (board_width, board_height), BACKGROUND)
    cursor_x = 0
    for normal, pressed in cards:
        board.paste(normal, (cursor_x, (board_height - normal.height) // 2), normal)
        cursor_x += normal.width + GAP
        board.paste(pressed, (cursor_x, (board_height - pressed.height) // 2), pressed)
        cursor_x += pressed.width + GAP

    board.save(OUTPUT_DIR / "pressed_state_preview_medium_compare.png")

if __name__ == "__main__":
    main()
