#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import shutil
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]

BACKUP_CANDIDATES = {
    "main.ftu": [
        ROOT / "Release" / "ui_debug_fix_backup_20260721_201748" / "main.ftu",
        ROOT / "Release" / "verify_runtime_push" / "main.ftu",
        ROOT / "backups" / "ui_full_20260720_131013" / "ui" / "main.ftu",
    ],
    "page1topset.ftu": [
        ROOT / "backups" / "ui_full_20260720_131013" / "ui" / "page1topset.ftu",
        ROOT / "Release" / "ide_close_guard_20260718_192026" / "ui" / "page1topset.ftu",
    ],
    "showsysdate.ftu": [
        ROOT / "backups" / "showsysdate_before_clickable_rows_20260720_160009" / "showsysdate.ftu",
        ROOT / "backups" / "showsysdate_before_picker_20260720_151708" / "showsysdate.ftu",
        ROOT / "Release" / "ide_close_guard_20260718_192026" / "ui" / "showsysdate.ftu",
    ],
}

TEXT_KEYS = ("取消", "ȡ��", "确定", "ȷ��")
CAPTION_KEYS = (
    "Cancel", "Cencel", "OK", "Ok", "Confirm",
    "cancel", "cencel", "ok", "confirm",
)


def is_target_button(node: dict) -> bool:
    caption = str(node.get("caption", ""))
    text = str(node.get("text", ""))
    return any(key in caption for key in CAPTION_KEYS) or any(key in text for key in TEXT_KEYS)


def walk_buttons(node: object, found: dict[tuple[str, int], dict]) -> None:
    if isinstance(node, dict):
        if is_target_button(node) and node.get("caption") is not None and node.get("id") is not None:
            found[(str(node.get("caption")), int(node.get("id")))] = node
        for value in node.values():
            walk_buttons(value, found)
    elif isinstance(node, list):
        for value in node:
            walk_buttons(value, found)


def button_visual_state(node: dict) -> dict:
    state = {}
    for key in ("picTab", "backgroundPic", "text", "colorTab", "fontSize", "bold", "iconPosition"):
        if key in node:
            state[key] = node[key]
    return state


def apply_visual_state(node: dict, state: dict) -> None:
    for key in ("picTab", "backgroundPic", "text", "colorTab", "fontSize", "bold", "iconPosition"):
        if key in node:
            del node[key]
    for key, value in state.items():
        node[key] = value


def select_backup(name: str) -> Path | None:
    for path in BACKUP_CANDIDATES.get(name, []):
        if path.is_file():
            return path
    return None


def restore_file(name: str) -> list[str]:
    current_path = UI_DIR / name
    backup_path = select_backup(name)
    if backup_path is None or not current_path.is_file():
        return []

    current_data, header, _ = decode_ftu(current_path)
    backup_data, _, _ = decode_ftu(backup_path)

    current_buttons: dict[tuple[str, int], dict] = {}
    backup_buttons: dict[tuple[str, int], dict] = {}
    walk_buttons(current_data, current_buttons)
    walk_buttons(backup_data, backup_buttons)

    changed: list[str] = []
    for key, current_button in current_buttons.items():
        backup_button = backup_buttons.get(key)
        if backup_button is None:
            continue
        before = button_visual_state(current_button)
        after = button_visual_state(backup_button)
        if before != after:
            apply_visual_state(current_button, after)
            changed.append(f"{name}: {key[0]}({key[1]})")

    if changed:
        save_dir = ROOT / "Release" / "restore_ok_cancel_backup"
        save_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(current_path, save_dir / name)
        current_path.write_bytes(encode_ftu(current_data, header))

    return changed


def main() -> None:
    all_changed: list[str] = []
    for name in BACKUP_CANDIDATES:
        all_changed.extend(restore_file(name))
    if not all_changed:
        print("no ok/cancel buttons changed")
        return
    print("restored:")
    for item in all_changed:
        print(item)


if __name__ == "__main__":
    main()
