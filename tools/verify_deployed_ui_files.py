#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[1]
ADB = Path(r"D:\Install\AndroidPlatformTools\adb.exe")
SERIAL = "192.168.1.70:5555"
REMOTE_UI = PurePosixPath("/mnt/extsd/ui")


def local_md5(path: Path) -> str:
    return hashlib.md5(path.read_bytes()).hexdigest()


def remote_path(path: Path) -> PurePosixPath:
    if path.parent == ROOT / "ui":
        return REMOTE_UI / path.name
    if ROOT / "resources" == path.parent or ROOT / "resources" in path.parents:
        return REMOTE_UI / PurePosixPath(path.relative_to(ROOT / "resources").as_posix())
    raise ValueError(f"not under ui/resources: {path}")


def remote_md5(path: PurePosixPath, temp_dir: Path) -> str:
    local_copy = temp_dir / path.name
    subprocess.run(
        [str(ADB), "-s", SERIAL, "pull", str(path), str(local_copy)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    return local_md5(local_copy)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("files", nargs="+")
    args = parser.parse_args()
    ok = True
    with tempfile.TemporaryDirectory(prefix="cj96_verify_deploy_") as temp:
        temp_dir = Path(temp)
        for name in args.files:
            local = (ROOT / name).resolve()
            remote = remote_path(local)
            lmd5 = local_md5(local)
            rmd5 = remote_md5(remote, temp_dir)
            same = lmd5 == rmd5
            ok = ok and same
            print(f"{'OK' if same else 'FAIL'} {name} local={lmd5} remote={rmd5} remote_path={remote}")
    if not ok:
        sys.exit(1)


if __name__ == "__main__":
    main()
