#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, decode_ftu


CONFIRM_CAPTIONS = (
    "OKButton", "OkButton", "ConfirmButton",
    "_OkButton", "_OKButton",
)
CANCEL_CAPTIONS = ("CancelButton", "CencelButton", "_CencelButton")
CONFIRM_TEXT = ("确定", "确认", "ȷ��")
CANCEL_TEXT = ("取消", "ȡ��")
EXCLUDE_CAPTIONS = (
    "TestAdressOkButton",
    "ChangeAdressOkButton",
    "DebugPasswordOkButton",
)


def role(node: dict) -> str | None:
    caption = str(node.get("caption", ""))
    text = str(node.get("text", ""))
    if caption in EXCLUDE_CAPTIONS:
        return None
    if any(key in caption for key in CANCEL_CAPTIONS) or text in CANCEL_TEXT:
        return "cancel"
    if any(key in caption for key in CONFIRM_CAPTIONS) or text in CONFIRM_TEXT:
        return "ok"
    return None


def walk(node: object, path: str, found: list[dict]) -> None:
    if isinstance(node, dict):
        item_role = role(node)
        if item_role:
            found.append({
                "role": item_role,
                "path": path,
                "caption": node.get("caption"),
                "id": node.get("id"),
                "position": node.get("position"),
                "text": node.get("text"),
                "picTab": node.get("picTab"),
                "backgroundPic": node.get("backgroundPic"),
            })
        for key, value in node.items():
            walk(value, f"{path}/{key}", found)
    elif isinstance(node, list):
        for index, value in enumerate(node):
            walk(value, f"{path}[{index}]", found)


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    all_found: dict[str, list[dict]] = {}
    for ftu in sorted(UI_DIR.glob("*.ftu")):
        data, _, _ = decode_ftu(ftu)
        found: list[dict] = []
        walk(data, "", found)
        if found:
            all_found[ftu.name] = found
    print(json.dumps(all_found, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
