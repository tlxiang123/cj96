#!/usr/bin/env python3
"""Compare the Quick Run inputs of the current tree and a saved snapshot."""

from __future__ import annotations

import hashlib
import argparse
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SNAPSHOT = ROOT.parent / "cj96_backups" / "current_project_before_desktop_backup_deploy_20260902_141051"
SINGLE_FILES = (
    Path("Release/libzkgui.so"),
    Path("font/Alibaba-PuHuiTi-Regular.ttf"),
    Path("Release/EasyUI.cfg"),
    Path("package.properties"),
    Path(".settings/com.zksw.flythings.easyui.prefs"),
)
TREE_DIRS = (Path("ui"), Path("resources"), Path("font"))


def md5(path: Path) -> str:
    if not path.is_file():
        return "MISSING"
    digest = hashlib.md5()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def tree_hashes(root: Path) -> dict[Path, str]:
    return {
        path.relative_to(root): md5(path)
        for path in root.rglob("*")
        if path.is_file()
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--snapshot", type=Path, default=SNAPSHOT)
    args = parser.parse_args()
    snapshot = args.snapshot
    if not snapshot.is_dir():
        raise SystemExit(f"Snapshot not found: {snapshot}")

    print("SINGLE FILES")
    for relative in SINGLE_FILES:
        old = md5(snapshot / relative)
        current = md5(ROOT / relative)
        print(f"{relative}: same={old == current} old={old} current={current}")

    print("\nTREE DELTAS")
    for relative in TREE_DIRS:
        old = tree_hashes(snapshot / relative)
        current = tree_hashes(ROOT / relative)
        only_current = sorted(current.keys() - old.keys())
        only_old = sorted(old.keys() - current.keys())
        changed = sorted(key for key in old.keys() & current.keys() if old[key] != current[key])
        print(
            f"{relative}: old_files={len(old)} current_files={len(current)} "
            f"only_current={len(only_current)} only_old={len(only_old)} changed={len(changed)}"
        )
        for label, values in (("current_only", only_current), ("old_only", only_old), ("changed", changed)):
            if values:
                print(f"  {label}: " + ", ".join(str(value) for value in values))


if __name__ == "__main__":
    main()
