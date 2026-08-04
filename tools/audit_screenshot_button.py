#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ftu_style import UI_DIR, decode_ftu


ROOT = Path(__file__).resolve().parents[1]


def walk(node: object, path: str = "", out: list[tuple[str, dict]] | None = None) -> list[tuple[str, dict]]:
    if out is None:
        out = []
    if isinstance(node, dict):
        if "caption" in node:
            out.append((path, node))
        for key, value in node.items():
            walk(value, f"{path}/{key}", out)
    elif isinstance(node, list):
        for index, value in enumerate(node):
            walk(value, f"{path}[{index}]", out)
    return out


def rect(node: dict) -> tuple[int, int, int, int] | None:
    pos = node.get("position")
    if not isinstance(pos, dict):
        return None
    try:
        left = int(pos.get("left", 0))
        top = int(pos.get("top", 0))
        width = int(pos.get("width", 0))
        height = int(pos.get("height", 0))
        return left, top, left + width, top + height
    except Exception:
        return None


def overlap(a: tuple[int, int, int, int], b: tuple[int, int, int, int]) -> bool:
    return a[0] < b[2] and a[2] > b[0] and a[1] < b[3] and a[3] > b[1]


def main() -> None:
    sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
    main_data, _, _ = decode_ftu(UI_DIR / "main.ftu")
    controls = walk(main_data)
    shot_items = [(path, node) for path, node in controls if "screenshot" in str(node.get("caption", "")).lower()]
    report: dict[str, object] = {"screenshot_controls": []}
    for shot_path, shot in shot_items:
        shot_rect = rect(shot)
        later_overlaps = []
        seen = False
        for path, node in controls:
            if path == shot_path:
                seen = True
                continue
            if not seen:
                continue
            node_rect = rect(node)
            if shot_rect and node_rect and overlap(shot_rect, node_rect):
                later_overlaps.append({
                    "path": path,
                    "caption": node.get("caption"),
                    "id": node.get("id"),
                    "position": node.get("position"),
                    "backgroundPic": node.get("backgroundPic"),
                    "picTab": node.get("picTab"),
                    "touchable": node.get("touchable"),
                })
        report["screenshot_controls"].append({
            "path": shot_path,
            "caption": shot.get("caption"),
            "id": shot.get("id"),
            "position": shot.get("position"),
            "picTab": shot.get("picTab"),
            "backgroundPic": shot.get("backgroundPic"),
            "touchable": shot.get("touchable"),
            "text": shot.get("text"),
            "later_overlaps": later_overlaps,
        })
    print(json.dumps(report, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
