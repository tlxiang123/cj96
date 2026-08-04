#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Guard FlyThingsIDE synchronization for cj96.

This script is intentionally conservative:
1. Verifies that required controls still exist in ui/main.ftu.
2. Removes stale Eclipse/FlyThingsIDE editor restore entries for main.ftu so
   the IDE must reopen the FTU from disk instead of restoring an old editor tab.
3. Lists unreferenced PNGs in ui/ so they can be archived safely.
"""

from __future__ import annotations

import argparse
import hashlib
import shutil
import sys
import time
import xml.etree.ElementTree as ET
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
WORKSPACE = ROOT.parent
MAIN_FTU = ROOT / "ui" / "main.ftu"
WORKBENCH_XMI = WORKSPACE / ".metadata" / ".plugins" / "org.eclipse.e4.workbench" / "workbench.xmi"


def die(msg: str) -> None:
    print(f"ERROR: {msg}", file=sys.stderr)
    raise SystemExit(1)


def local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def file_md5(path: Path) -> str:
    h = hashlib.md5()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def load_ftu():
    sys.path.insert(0, str(ROOT / "tools"))
    import ftu_style  # type: ignore

    return ftu_style.decode_ftu(MAIN_FTU)[0]


def walk(node, path: str = ""):
    if isinstance(node, dict):
        yield path, node
        for k, v in node.items():
            yield from walk(v, f"{path}/{k}")
    elif isinstance(node, list):
        for i, v in enumerate(node):
            yield from walk(v, f"{path}[{i}]")


def find_by_caption(data, caption: str):
    return [(p, n) for p, n in walk(data) if isinstance(n, dict) and n.get("caption") == caption]


def verify_required_controls() -> bool:
    if not MAIN_FTU.exists():
        die(f"missing {MAIN_FTU}")

    data = load_ftu()
    ok = True
    required = {
        "sys_back": {"id": 100, "position": {"left": 0, "top": 0, "width": 87, "height": 70}},
        "CycleCancelButton": {"id": 20110, "position": {"left": 650, "top": 292, "width": 120, "height": 60}},
        "CycleOKButton": {"id": 20076},
        "SetRunTimeCencelButton": {"id": 20048, "position": {"width": 120, "height": 60}},
        "SetRunTimeZeroButton": {"id": 20049, "position": {"width": 120, "height": 60}},
        "SetRunTimeOKButton": {"id": 20047, "position": {"width": 120, "height": 60}},
    }

    print(f"main.ftu md5={file_md5(MAIN_FTU)} mtime={time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(MAIN_FTU.stat().st_mtime))}")
    for caption, expected in required.items():
        hits = find_by_caption(data, caption)
        if not hits:
            print(f"MISS {caption}")
            ok = False
            continue
        path, node = hits[0]
        pos = node.get("position") or {}
        pic = node.get("picTab") or node.get("backgroundPic")
        print(f"OK   {caption} path={path} id={node.get('id')} pos={pos} pic={pic}")
        if expected.get("id") is not None and node.get("id") != expected["id"]:
            print(f"  BAD_ID expected={expected['id']} actual={node.get('id')}")
            ok = False
        for key, val in expected.get("position", {}).items():
            if pos.get(key) != val:
                print(f"  BAD_POS {key} expected={val} actual={pos.get(key)}")
                ok = False
    return ok


def element_contains_text(elem: ET.Element, needle: str) -> bool:
    if needle in elem.tag:
        return True
    for k, v in elem.attrib.items():
        if needle in k or needle in v:
            return True
    if elem.text and needle in elem.text:
        return True
    if elem.tail and needle in elem.tail:
        return True
    for child in list(elem):
        if element_contains_text(child, needle):
            return True
    return False


def is_editor_restore_entry(elem: ET.Element) -> bool:
    name = local_name(elem.tag)
    if name == "file":
        return True
    if name == "children":
        return elem.attrib.get("elementId") == "org.eclipse.e4.ui.compatibility.editor" or any(
            "DiagramEditor" in v for v in elem.attrib.values()
        )
    return False


def clear_main_ftu_editor_restore() -> int:
    if not WORKBENCH_XMI.exists():
        print(f"SKIP metadata not found: {WORKBENCH_XMI}")
        return 0

    backup_dir = ROOT / "Release" / f"ide_workbench_backup_{time.strftime('%Y%m%d_%H%M%S')}"
    backup_dir.mkdir(parents=True, exist_ok=True)
    backup_path = backup_dir / "workbench.xmi"
    shutil.copy2(WORKBENCH_XMI, backup_path)

    ET.register_namespace("xmi", "http://www.omg.org/XMI")
    ET.register_namespace("xsi", "http://www.w3.org/2001/XMLSchema-instance")
    tree = ET.parse(WORKBENCH_XMI)
    root = tree.getroot()

    needles = ["/cj96/ui/main.ftu", "cj96/ui/main.ftu"]
    removed = 0

    def prune(parent: ET.Element) -> None:
        nonlocal removed
        for child in list(parent):
            if is_editor_restore_entry(child) and any(element_contains_text(child, n) for n in needles):
                parent.remove(child)
                removed += 1
            else:
                prune(child)

    prune(root)

    if removed:
        tree.write(WORKBENCH_XMI, encoding="UTF-8", xml_declaration=True)
    else:
        backup_path.unlink(missing_ok=True)
        try:
            backup_dir.rmdir()
        except OSError:
            pass
    print(f"workbench_removed={removed}")
    if removed:
        print(f"workbench_backup={backup_path}")
    return removed


def collect_referenced_pngs() -> set[str]:
    refs: set[str] = set()

    def add_from_value(value) -> None:
        if isinstance(value, str):
            if value.lower().endswith(".png"):
                refs.add(Path(value.replace("\\", "/")).name)
        elif isinstance(value, dict):
            for item in value.values():
                add_from_value(item)
        elif isinstance(value, list):
            for item in value:
                add_from_value(item)

    for ftu in (ROOT / "ui").glob("*.ftu"):
        try:
            sys.path.insert(0, str(ROOT / "tools"))
            import ftu_style  # type: ignore

            data = ftu_style.decode_ftu(ftu)[0]
            add_from_value(data)
        except Exception as exc:
            print(f"WARN could not decode {ftu}: {exc}")

    text_exts = {".ftu", ".cpp", ".cc", ".c", ".h", ".hpp", ".xml", ".cfg", ".ini", ".mk", ".txt"}
    for p in ROOT.rglob("*"):
        if not p.is_file() or p.suffix.lower() not in text_exts:
            continue
        if "Release" in p.parts or "backups" in p.parts:
            continue
        try:
            s = p.read_text(encoding="utf-8", errors="ignore")
        except Exception:
            continue
        for part in s.replace("\\", "/").split():
            if ".png" not in part:
                continue
            cleaned = part.strip("\"'<>(),;:{}[]")
            if cleaned.lower().endswith(".png"):
                refs.add(Path(cleaned).name)
    return refs


def list_unref_ui_pngs(limit: int = 80) -> None:
    refs = collect_referenced_pngs()
    pngs = sorted((ROOT / "ui").glob("*.png"), key=lambda p: p.stat().st_mtime, reverse=True)
    unref = [p for p in pngs if p.name not in refs]
    print(f"ui_png_total={len(pngs)} referenced_by_sources={len(refs)} unreferenced_in_ui={len(unref)}")
    for p in unref[:limit]:
        print(f"UNREF {p.name} {p.stat().st_size}")
    if len(unref) > limit:
        print(f"... {len(unref) - limit} more")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--clear-main-editor-cache", action="store_true")
    parser.add_argument("--list-unref-ui-png", action="store_true")
    args = parser.parse_args()

    ok = verify_required_controls()
    if args.clear_main_editor_cache:
        clear_main_ftu_editor_restore()
    if args.list_unref_ui_png:
        list_unref_ui_pngs()
    return 0 if ok else 2


if __name__ == "__main__":
    raise SystemExit(main())
