from __future__ import annotations

import json
import shutil
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_ROOT = ROOT / "Release" / f"sdcard_runtime_{time.strftime('%Y%m%d_%H%M%S')}"


def copy_tree(src: Path, dst: Path) -> None:
    if src.exists():
        shutil.copytree(src, dst, dirs_exist_ok=True)


def write_easyui_cfg(dst: Path) -> None:
    cfg_path = ROOT / "Release" / "EasyUI.cfg"
    if cfg_path.exists():
        cfg = json.loads(cfg_path.read_text(encoding="utf-8"))
    else:
        cfg = {}
    cfg.update(
        {
            "startupLibPath": "/mnt/extsd/lib/libzkgui.so",
            "languagePath": "/mnt/extsd/tr/",
            "font": "/mnt/extsd/font/Alibaba-PuHuiTi-Regular.ttf",
            "resPath": "/mnt/extsd/ui/",
        }
    )
    dst.write_text(json.dumps(cfg, ensure_ascii=False, indent=4), encoding="utf-8")


def size_tree(path: Path) -> int:
    return sum(f.stat().st_size for f in path.rglob("*") if f.is_file())


def main() -> int:
    if OUT_ROOT.exists():
        shutil.rmtree(OUT_ROOT)
    OUT_ROOT.mkdir(parents=True)

    write_easyui_cfg(OUT_ROOT / "EasyUI.cfg")
    (OUT_ROOT / "lib").mkdir()
    shutil.copy2(ROOT / "Release" / "libzkgui.so", OUT_ROOT / "lib" / "libzkgui.so")

    copy_tree(ROOT / "ui", OUT_ROOT / "ui")
    copy_tree(ROOT / "resources", OUT_ROOT / "resources")

    (OUT_ROOT / "font").mkdir()
    shutil.copy2(
        ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf",
        OUT_ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf",
    )

    tr_src = ROOT / "tr"
    tr_dst = OUT_ROOT / "tr"
    if tr_src.exists():
        copy_tree(tr_src, tr_dst)
    else:
        tr_dst.mkdir()
        (tr_dst / "zh_CN-简体中文.json").write_text("{}\n", encoding="utf-8")
        (tr_dst / "zh_TW-繁體中文.json").write_text("{}\n", encoding="utf-8")
        (tr_dst / "en_US-ENGLISH.json").write_text("{}\n", encoding="utf-8")

    print(f"output_dir={OUT_ROOT}")
    print(f"total_size={size_tree(OUT_ROOT)}")
    for name in ["EasyUI.cfg", "lib", "ui", "resources", "font", "tr"]:
        path = OUT_ROOT / name
        print(f"{name}={size_tree(path) if path.is_dir() else path.stat().st_size}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
