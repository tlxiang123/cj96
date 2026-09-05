#!/usr/bin/env python3
"""Add the remote-upgrade card and confirmation dialog to page1topset.ftu."""

from __future__ import annotations

import re
import shutil
import time
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "ui" / "page1topset.ftu"
RESOURCE_DIR = ROOT / "resources"
DESKTOP_IMAGE = Path.home() / "Desktop" / "update.img"
USER_ICON_SOURCE = Path(
    r"C:\Users\Administrator\AppData\Local\Temp\codex-clipboard-78ab7c5e-e69c-41fe-a41f-ef7fdb4792de.png"
)

SCALE = 4
BLUE = 0x168BFF
INK = 0x005BBB
MUTED = 0x526579
WHITE = 0xFFFFFF

REMOTE_UPGRADE_BUTTON_ID = 20070
REMOTE_UPGRADE_CONFIRM_ID = 20071
REMOTE_UPGRADE_CANCEL_ID = 20072
REMOTE_UPGRADE_TITLE_ID = 50070
REMOTE_UPGRADE_VERSION_ID = 50071
REMOTE_UPGRADE_CONTENT_ID = 50072
REMOTE_UPGRADE_HINT_ID = 50073

CARD_NAME = "topset_remote_upgrade_113.png"
DIALOG_NAME = "remote_upgrade_dialog_595x269.png"
PREVIEW_NAME = "remote_upgrade_dialog_preview.png"


def walk(node: object):
    if isinstance(node, dict):
        yield node
        for value in node.values():
            yield from walk(value)
    elif isinstance(node, list):
        for value in node:
            yield from walk(value)


def find_control(node: object, caption: str) -> tuple[dict, str, dict]:
    if not isinstance(node, dict):
        raise LookupError(caption)
    for key, value in node.items():
        if not isinstance(value, dict):
            continue
        if value.get("caption") == caption:
            return node, key, value
        try:
            return find_control(value, caption)
        except LookupError:
            pass
    raise LookupError(caption)


def next_key(root: dict, parent: dict, prefix: str) -> str:
    suffixes = [
        int(match.group(1))
        for node in walk(root)
        if isinstance(node, dict)
        for key in node
        if (match := re.fullmatch(rf"{re.escape(prefix)}__(\d+)", key))
    ]
    suffix = max(suffixes, default=0) + 1
    while f"{prefix}__{suffix}" in parent:
        suffix += 1
    return f"{prefix}__{suffix}"


def assert_ids_available(data: dict) -> None:
    used = {
        node["id"]: node.get("caption", "")
        for node in walk(data)
        if isinstance(node, dict) and isinstance(node.get("id"), int)
    }
    requested = {
        REMOTE_UPGRADE_BUTTON_ID: "RemoteUpgradeButton",
        REMOTE_UPGRADE_CONFIRM_ID: "RemoteUpgradeConfirmButton",
        REMOTE_UPGRADE_CANCEL_ID: "RemoteUpgradeCancelButton",
        REMOTE_UPGRADE_TITLE_ID: "RemoteUpgradeTitleText",
        REMOTE_UPGRADE_VERSION_ID: "RemoteUpgradeVersionText",
        REMOTE_UPGRADE_CONTENT_ID: "RemoteUpgradeContentText",
        REMOTE_UPGRADE_HINT_ID: "RemoteUpgradeHintText",
    }
    for control_id, caption in requested.items():
        existing = used.get(control_id)
        if existing is not None and existing != caption:
            raise RuntimeError(f"ID {control_id} is already used by {existing}")


def font(size: int) -> ImageFont.FreeTypeFont:
    candidates = (
        ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf",
        Path(r"C:\Windows\Fonts\msyh.ttc"),
    )
    for candidate in candidates:
        if candidate.is_file():
            return ImageFont.truetype(str(candidate), size)
    raise FileNotFoundError("No Chinese font is available for the remote-upgrade icon")


def make_card() -> Image.Image:
    if not USER_ICON_SOURCE.is_file():
        raise FileNotFoundError(f"The supplied remote-upgrade icon is missing: {USER_ICON_SOURCE}")

    source = Image.open(USER_ICON_SOURCE).convert("RGBA")
    # The supplied source is a 288 x 249 screenshot rather than a square icon.
    # Its actual 222 x 222 card is on the left; the remaining pixels on the
    # right are only screenshot background/shadow and must never be scaled into
    # the 113 x 113 settings card.
    card_left = 18
    card_top = 18
    card_size = 222
    card = source.crop((card_left, card_top, card_left + card_size, card_top + card_size))

    # The supplied capture has a faint neutral-grey watermark confined to its
    # upper-left corner. Remove only neutral grey pixels there; the blue rounded
    # border is preserved because it has a clear blue channel bias.
    pixels = card.load()
    watermark_width = round(card.width * 0.24)
    watermark_height = round(card.height * 0.16)
    for y in range(watermark_height):
        for x in range(watermark_width):
            red, green, blue, alpha = pixels[x, y]
            if alpha and min(red, green, blue) >= 175 and max(red, green, blue) - min(red, green, blue) <= 8:
                pixels[x, y] = (255, 255, 255, alpha)

    # Use the exact alpha treatment of an existing settings card.  This keeps
    # the supplied RGB artwork intact while making its outside pixels, rounded
    # corners, and anti-aliased edge blend with the settings page exactly like
    # the Display/Debug cards instead of showing a white screenshot rectangle.
    card = card.resize((113, 113), Image.Resampling.LANCZOS)
    alpha_template = Image.open(RESOURCE_DIR / "topset_debug_113.png").convert("RGBA")
    card.putalpha(alpha_template.getchannel("A"))
    return card


def make_dialog() -> Image.Image:
    width, height = 595, 269
    image = Image.new("RGBA", (width * SCALE, height * SCALE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle(
        (2 * SCALE, 2 * SCALE, width * SCALE - 3 * SCALE, height * SCALE - 3 * SCALE),
        radius=16 * SCALE,
        fill=(249, 253, 255, 255),
        outline=(22, 139, 255, 255),
        width=3 * SCALE,
    )
    return image.resize((width, height), Image.Resampling.LANCZOS)


def make_dialog_preview(version: str) -> None:
    dialog = Image.open(RESOURCE_DIR / DIALOG_NAME).convert("RGBA")
    draw = ImageDraw.Draw(dialog)
    title_font = font(28)
    text_font = font(20)
    hint_font = font(18)
    title = "远程升级"
    title_box = draw.textbbox((0, 0), title, font=title_font)
    draw.text(((dialog.width - (title_box[2] - title_box[0])) // 2, 22), title,
              font=title_font, fill=(0, 91, 187, 255))
    draw.text((62, 78), f"升级版本：{version}", font=text_font, fill=(0, 91, 187, 255))
    draw.text((62, 116), "更新内容：/cj96/ui/page1topset.ftu",
              font=text_font, fill=(0, 91, 187, 255))
    draw.text((62, 153), "请确认是否升级", font=hint_font, fill=(82, 101, 121, 255))
    for name, left in (("w2_group_bind_cancel_120x60.png", 143), ("w2_ok_120x60.png", 332)):
        dialog.alpha_composite(Image.open(RESOURCE_DIR / name).convert("RGBA"), (left, 196))
    dialog.save(ROOT / "Release" / PREVIEW_NAME, optimize=True)


def read_upgrade_version() -> str:
    if not DESKTOP_IMAGE.is_file():
        raise FileNotFoundError(f"Desktop update image is missing: {DESKTOP_IMAGE}")
    header = DESKTOP_IMAGE.read_bytes()[:256]
    match = re.search(rb"ZKSWE(V[0-9][A-Za-z0-9._-]*)", header)
    if match is None:
        raise RuntimeError("Could not read the version from Desktop update.img")
    return match.group(1).decode("ascii")


def text_node(caption: str, control_id: int, text: str, left: int, top: int,
              width: int, height: int, font_size: int, color: int, bold: bool = False) -> dict:
    node = {
        "alignment": 37,
        "caption": caption,
        "colorTab": {"color0": color},
        "family": "Alibaba-PuHuiTi-Regular",
        "fontSize": font_size,
        "id": control_id,
        "position": {"height": height, "left": left, "top": top, "width": width},
        "text": text,
        "touchable": False,
    }
    if bold:
        node["bold"] = True
    return node


def image_button(caption: str, control_id: int, left: int, top: int, image_name: str) -> dict:
    return {
        "alignment": 37,
        "beepEnable": True,
        "caption": caption,
        "id": control_id,
        "picTab": {"pic0": image_name, "pic1": image_name, "pic2": image_name},
        "position": {"height": 60, "left": left, "top": top, "width": 120},
    }


def main() -> None:
    version = read_upgrade_version()
    make_card().save(RESOURCE_DIR / CARD_NAME, optimize=True)
    make_dialog().save(RESOURCE_DIR / DIALOG_NAME, optimize=True)
    make_dialog_preview(version)

    data, header, _ = decode_ftu(UI_PATH)
    assert_ids_available(data)
    _, _, frame = find_control(data, "TopSetContentFrameWindow")
    _, _, dialog = find_control(frame, "update_wnd")

    for key, value in list(frame.items()):
        if isinstance(value, dict) and value.get("caption") == "RemoteUpgradeButton":
            del frame[key]
    for key, value in list(dialog.items()):
        if isinstance(value, dict):
            del dialog[key]

    dialog.update({
        "backgroundPic": DIALOG_NAME,
        "beepEnable": True,
        "modal": True,
        "visible": False,
        "position": {"height": 269, "left": 186, "top": 45, "width": 595},
    })
    dialog[next_key(data, dialog, "textview")] = text_node(
        "RemoteUpgradeTitleText", REMOTE_UPGRADE_TITLE_ID, "远程升级",
        0, 22, 595, 38, 28, INK, True,
    )
    dialog[next_key(data, dialog, "textview")] = text_node(
        "RemoteUpgradeVersionText", REMOTE_UPGRADE_VERSION_ID, f"升级版本：{version}",
        62, 78, 471, 30, 20, INK,
    )
    dialog[next_key(data, dialog, "textview")] = text_node(
        "RemoteUpgradeContentText", REMOTE_UPGRADE_CONTENT_ID,
        "更新内容：/cj96/ui/page1topset.ftu",
        62, 116, 471, 30, 20, INK,
    )
    dialog[next_key(data, dialog, "textview")] = text_node(
        "RemoteUpgradeHintText", REMOTE_UPGRADE_HINT_ID, "请确认是否升级",
        62, 153, 471, 28, 18, MUTED,
    )
    dialog[next_key(data, dialog, "button")] = image_button(
        "RemoteUpgradeCancelButton", REMOTE_UPGRADE_CANCEL_ID,
        143, 196, "w2_group_bind_cancel_120x60.png",
    )
    dialog[next_key(data, dialog, "button")] = image_button(
        "RemoteUpgradeConfirmButton", REMOTE_UPGRADE_CONFIRM_ID,
        332, 196, "w2_ok_120x60.png",
    )

    frame[next_key(data, frame, "button")] = {
        "alignment": 37,
        "beepEnable": True,
        "caption": "RemoteUpgradeButton",
        "id": REMOTE_UPGRADE_BUTTON_ID,
        "picTab": {"pic0": CARD_NAME, "pic1": CARD_NAME, "pic2": CARD_NAME},
        "position": {"height": 113, "left": 404, "top": 230, "width": 113},
    }

    stamp = time.strftime("%Y%m%d_%H%M%S")
    backup = ROOT / "Release" / f"page1topset_remote_upgrade_{stamp}.ftu"
    shutil.copy2(UI_PATH, backup)
    UI_PATH.write_bytes(encode_ftu(data, header))

    decoded, _, _ = decode_ftu(UI_PATH)
    _, _, verified_frame = find_control(decoded, "TopSetContentFrameWindow")
    _, _, verified_dialog = find_control(verified_frame, "update_wnd")
    _, _, verified_button = find_control(verified_frame, "RemoteUpgradeButton")
    if verified_button["position"] != {"height": 113, "left": 404, "top": 230, "width": 113}:
        raise RuntimeError("remote-upgrade card position verification failed")
    if verified_dialog.get("visible") is not False:
        raise RuntimeError("remote-upgrade dialog must be hidden initially")
    if find_control(verified_dialog, "RemoteUpgradeContentText")[2]["text"] != "更新内容：/cj96/ui/page1topset.ftu":
        raise RuntimeError("remote-upgrade content path verification failed")
    print(f"version={version}")
    print(f"backup={backup}")
    print(f"updated={UI_PATH}")


if __name__ == "__main__":
    main()
