#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import os
import time


DESKTOP = Path(r"C:\Users\Administrator\Desktop")


def main() -> None:
    for path in sorted(DESKTOP.glob("cj96*")):
        stat = path.stat()
        kind = "DIR" if path.is_dir() else "FILE"
        print(f"{kind} {path.name} modified={time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(stat.st_mtime))}")
        if path.is_dir():
            children = list(path.iterdir())
            print(f"  items={len(children)}")
            for child in sorted(children)[:12]:
                cstat = child.stat()
                print(f"  - {child.name} {'DIR' if child.is_dir() else 'FILE'} size={cstat.st_size}")


if __name__ == "__main__":
    main()
