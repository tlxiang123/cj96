#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, RESOURCE_DIR, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
BACKUP_DIR = ROOT / "Release" / f"before_ok_cancel_cycle_fix_{time.strftime('%Y%m%d_%H%M%S')}"

OK_PIC = "w2_ok_120x60.png"
CANCEL_PIC = "w2_group_bind_cancel_120x60.png"

CONFIRM_CAPTION_KEYS = ("OKButton", "OkButton", "ConfirmButton", "_OKButton", "_OkButton")
CANCEL_CAPTION_KEYS = ("CancelButton", "CencelButton", "_CencelButton")
CONFIRM_TEXTS = {"确定", "确认", "ȷ��"}
CANCEL_TEXTS = {"取消", "ȡ��"}

EXCLUDE_CAPTIONS = {
    "TestAdressOkButton",
    "ChangeAdressOkButton",
}


def find_caption(node: object, caption: str) -> dict | None:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            found = find_caption(value, caption)
            if found is not None:
                return found
    elif isinstance(node, list):
        for value in node:
            found = find_caption(value, caption)
            if found is not None:
                return found
    return None


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


def set_button_pic(node: dict, pic: str) -> None:
    position = node.get("position", {})
    width = int(position.get("width", 120))
    height = int(position.get("height", 60))
    node["picTab"] = {"pic0": pic, "pic1": pic, "pic2": pic}
    node["iconPosition"] = {"left": 0, "top": 0, "width": width, "height": height}
    node["text"] = ""


def update_buttons(node: object, changes: list[str], ftu_name: str) -> None:
    if isinstance(node, dict):
        role = button_role(node)
        if role:
            pic = OK_PIC if role == "ok" else CANCEL_PIC
            before = node.get("picTab")
            set_button_pic(node, pic)
            if before != node.get("picTab"):
                changes.append(f"{ftu_name}: {node.get('caption')}({node.get('id')}) -> {pic}")
        for value in node.values():
            update_buttons(value, changes, ftu_name)
    elif isinstance(node, list):
        for value in node:
            update_buttons(value, changes, ftu_name)


def make_region(caption: str, control_id: int, left: int, top: int,
                width: int, height: int, pic: str) -> dict:
    return {
        "caption": caption,
        "id": control_id,
        "backgroundPic": pic,
        "touchable": False,
        "position": {"left": left, "top": top, "width": width, "height": height},
    }


def add_cycle_frames(data: dict, changes: list[str]) -> None:
    cycle = find_caption(data, "CycleWindow")
    if cycle is None:
        return

    scalar_items: list[tuple[str, object]] = []
    child_items: list[tuple[str, object]] = []
    removed = False
    for key, value in cycle.items():
        if isinstance(value, dict):
            if value.get("caption") in {"CycleRegionLeftWindow", "CycleRegionRightWindow"}:
                removed = True
                continue
            child_items.append((key, value))
        else:
            scalar_items.append((key, value))

    left_region = make_region(
        "CycleRegionLeftWindow", 110061, 27, 32, 390, 340,
        "cycle_region_left_390x340.png",
    )
    right_region = make_region(
        "CycleRegionRightWindow", 110062, 462, 32, 545, 340,
        "cycle_region_right_545x340.png",
    )

    cycle.clear()
    for key, value in scalar_items:
        cycle[key] = value
    cycle["window__cycle_region_left"] = left_region
    cycle["window__cycle_region_right"] = right_region
    for key, value in child_items:
        cycle[key] = value

    changes.append(
        "main.ftu: CycleWindow frames -> left(27,32,390,340), right(462,32,545,340)"
        + (" replaced existing" if removed else "")
    )


def backup_current() -> None:
    (BACKUP_DIR / "ui").mkdir(parents=True, exist_ok=True)
    (BACKUP_DIR / "resources").mkdir(parents=True, exist_ok=True)
    for path in UI_DIR.glob("*.ftu"):
        shutil.copy2(path, BACKUP_DIR / "ui" / path.name)
    for name in [
        OK_PIC,
        CANCEL_PIC,
        "cycle_region_left_390x340.png",
        "cycle_region_right_545x340.png",
    ]:
        source = RESOURCE_DIR / name
        if source.is_file():
            shutil.copy2(source, BACKUP_DIR / "resources" / name)


def main() -> None:
    required = [
        OK_PIC,
        CANCEL_PIC,
        "cycle_region_left_390x340.png",
        "cycle_region_right_545x340.png",
    ]
    missing = [name for name in required if not (RESOURCE_DIR / name).is_file()]
    if missing:
        raise FileNotFoundError(", ".join(missing))

    backup_current()
    all_changes: list[str] = []
    for path in sorted(UI_DIR.glob("*.ftu")):
        data, header, _ = decode_ftu(path)
        changes: list[str] = []
        update_buttons(data, changes, path.name)
        if path.name == "main.ftu":
            add_cycle_frames(data, changes)
        if changes:
            path.write_bytes(encode_ftu(data, header))
            decoded, _, _ = decode_ftu(path)
            if decoded != data:
                raise RuntimeError(f"round-trip verification failed: {path}")
            all_changes.extend(changes)

    print(f"backup={BACKUP_DIR}")
    for item in all_changes:
        print(item)


if __name__ == "__main__":
    main()
