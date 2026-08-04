#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

from PIL import Image

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, RESOURCE_DIR, decode_ftu


OK_PIC = "w2_ok_120x60.png"
CANCEL_PIC = "w2_group_bind_cancel_120x60.png"
BAD_BUTTON_PICS = {
    "cycle_ok_confirm_120x60.png",
    "w2_cancel_120x60.png",
    "w2_group_bind_confirm_120x60.png",
    "w2_group_rename_confirm_120x60.png",
}

CONFIRM_CAPTION_KEYS = ("OKButton", "OkButton", "ConfirmButton", "_OKButton", "_OkButton")
CANCEL_CAPTION_KEYS = ("CancelButton", "CencelButton", "_CencelButton")
CONFIRM_TEXTS = {"确定", "确认", "ȷ��"}
CANCEL_TEXTS = {"取消", "ȡ��"}
EXCLUDE_CAPTIONS = {
    "TestAdressOkButton",
    "ChangeAdressOkButton",
}

CYCLE_FRAME_EXPECTED = {
    "CycleRegionLeftWindow": {
        "position": {"left": 27, "top": 32, "width": 390, "height": 340},
        "backgroundPic": "cycle_region_left_390x340.png",
    },
    "CycleRegionRightWindow": {
        "position": {"left": 462, "top": 32, "width": 545, "height": 340},
        "backgroundPic": "cycle_region_right_545x340.png",
    },
}


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def button_role(node: dict) -> str | None:
    caption = str(node.get("caption", ""))
    text = str(node.get("text", ""))
    if caption in EXCLUDE_CAPTIONS:
        return None
    if any(key in caption for key in CANCEL_CAPTION_KEYS) or text in CANCEL_TEXTS:
        return "cancel"
    if any(key in caption for key in CONFIRM_CAPTION_KEYS) or text in CONFIRM_TEXTS:
        return "ok"
    return None


def walk(node: object, found: list[tuple[str, dict]], path: str = "") -> None:
    if isinstance(node, dict):
        caption = node.get("caption")
        if caption is not None:
            found.append((path, node))
        for key, value in node.items():
            walk(value, found, f"{path}/{key}")
    elif isinstance(node, list):
        for index, value in enumerate(node):
            walk(value, found, f"{path}[{index}]")


def assert_asset(name: str) -> dict:
    path = RESOURCE_DIR / name
    if not path.is_file():
        fail(f"missing resource {name}")
    with Image.open(path) as image:
        rgba = image.convert("RGBA")
        if rgba.size != (120, 60):
            fail(f"{name} size is {rgba.size}, expected 120x60")
        corners = [
            rgba.getpixel((0, 0)),
            rgba.getpixel((119, 0)),
            rgba.getpixel((0, 59)),
            rgba.getpixel((119, 59)),
        ]
        if any(pixel[3] != 0 for pixel in corners):
            fail(f"{name} corners are not transparent: {corners}")
        alpha = rgba.getchannel("A")
        if alpha.getbbox() is None:
            fail(f"{name} is fully transparent")
        opaque_pixels = sum(1 for value in alpha.getdata() if value == 255)
        transparent_pixels = sum(1 for value in alpha.getdata() if value == 0)
        return {
            "name": name,
            "size": list(rgba.size),
            "corner_alpha": [pixel[3] for pixel in corners],
            "opaque_pixels": opaque_pixels,
            "transparent_pixels": transparent_pixels,
        }


def assert_button(ftu_name: str, path: str, node: dict, role: str) -> dict:
    expected_pic = OK_PIC if role == "ok" else CANCEL_PIC
    caption = node.get("caption")
    pic_tab = node.get("picTab")
    if not isinstance(pic_tab, dict):
        fail(f"{ftu_name} {caption} has no picTab")
    for key in ("pic0", "pic1", "pic2"):
        if pic_tab.get(key) != expected_pic:
            fail(f"{ftu_name} {caption} {key}={pic_tab.get(key)!r}, expected {expected_pic!r}")
    used_bad = sorted(set(pic_tab.values()).intersection(BAD_BUTTON_PICS))
    if used_bad:
        fail(f"{ftu_name} {caption} still uses bad image(s): {used_bad}")
    if node.get("text", "") not in ("", None):
        fail(f"{ftu_name} {caption} still has text={node.get('text')!r}")
    pos = node.get("position") or {}
    icon_pos = node.get("iconPosition") or {}
    for key in ("width", "height"):
        if int(icon_pos.get(key, -1)) != int(pos.get(key, -2)):
            fail(f"{ftu_name} {caption} iconPosition.{key} does not match position.{key}")
    return {
        "file": ftu_name,
        "caption": caption,
        "role": role,
        "position": pos,
        "pic": expected_pic,
        "path": path,
    }


def assert_cycle_frames(data: dict) -> list[dict]:
    controls: list[tuple[str, dict]] = []
    walk(data, controls)
    by_caption = {str(node.get("caption")): node for _, node in controls}
    out: list[dict] = []
    for caption, expected in CYCLE_FRAME_EXPECTED.items():
        node = by_caption.get(caption)
        if node is None:
            fail(f"main.ftu missing {caption}")
        if node.get("backgroundPic") != expected["backgroundPic"]:
            fail(f"{caption} backgroundPic={node.get('backgroundPic')!r}")
        pos = node.get("position") or {}
        for key, value in expected["position"].items():
            if int(pos.get(key, -99999)) != value:
                fail(f"{caption} position.{key}={pos.get(key)!r}, expected {value}")
        out.append({"caption": caption, "position": pos, "backgroundPic": node.get("backgroundPic")})
    return out


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    asset_report = [assert_asset(OK_PIC), assert_asset(CANCEL_PIC)]
    button_report: list[dict] = []
    cycle_report: list[dict] = []

    for ftu in sorted(UI_DIR.glob("*.ftu")):
        data, _, _ = decode_ftu(ftu)
        controls: list[tuple[str, dict]] = []
        walk(data, controls)
        for path, node in controls:
            role = button_role(node)
            if role:
                button_report.append(assert_button(ftu.name, path, node, role))
        if ftu.name == "main.ftu":
            cycle_report = assert_cycle_frames(data)

    captions = {item["caption"] for item in button_report}
    required = {
        "CycleOKButton",
        "CycleCancelButton",
        "DebugPasswordOkButton",
        "DebugPasswordCancelButton",
    }
    missing = sorted(required - captions)
    if missing:
        fail(f"required buttons were not verified: {missing}")

    print(json.dumps({
        "assets": asset_report,
        "verified_button_count": len(button_report),
        "verified_buttons": button_report,
        "cycle_frames": cycle_report,
    }, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
