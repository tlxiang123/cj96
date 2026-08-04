from __future__ import annotations

import ast
import json
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

from fontTools import subset

ROOT = Path(__file__).resolve().parents[1]
PLUGIN = Path(r"D:\Install\FlyThingsIDE\bin\configuration\org.eclipse.osgi\455\0\.cp")
MK_SQUASHFS = PLUGIN / "bundle" / "bin" / "zkswe_mkimg.exe"
FSIMG = PLUGIN / "bundle" / "bin" / "fsimg.exe"

RES_PARTITION_SIZE = 0x00710000
OUT_ROOT = ROOT / "Release" / f"internal_update_{time.strftime('%Y%m%d_%H%M%S')}"
RES_ROOT = OUT_ROOT / "res_root"
OUTPUT_IMG = OUT_ROOT / "update.img"
OUTPUT_SQFS = OUT_ROOT / "res.sqfs"
SD_IMG = None

FONT_NAME = "Alibaba-PuHuiTi-Regular.ttf"
SOURCE_FONT = ROOT / "font" / FONT_NAME


def run(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(cmd, text=True, capture_output=True, timeout=180)
    if result.returncode != 0:
        raise RuntimeError(
            "command failed:\n"
            + " ".join(cmd)
            + "\nstdout:\n"
            + result.stdout
            + "\nstderr:\n"
            + result.stderr
        )
    return result


def copy_tree(src: Path, dst: Path) -> None:
    if not src.exists():
        return
    shutil.copytree(src, dst, dirs_exist_ok=True)


def collect_ftu_text_and_refs() -> tuple[str, set[str]]:
    sys.path.insert(0, str(ROOT / "tools"))
    from ftu_style import decode_ftu  # type: ignore

    texts: list[str] = []
    refs: set[str] = set()

    string_literal_re = re.compile(r'"((?:\\.|[^"\\])*)"|\'((?:\\.|[^\'\\])*)\'', re.DOTALL)

    def add_string_literals(text: str) -> None:
        for match in string_literal_re.finditer(text):
            literal = match.group(1) if match.group(1) is not None else match.group(2)
            if literal:
                try:
                    texts.append(ast.literal_eval(f'"{literal}"'))
                except Exception:
                    texts.append(literal)

    def walk(obj: object) -> None:
        if isinstance(obj, dict):
            for key, value in obj.items():
                if isinstance(value, str):
                    lower_key = key.lower()
                    if key in {"text", "caption"} and value:
                        texts.append(value)
                    if (
                        lower_key.endswith("picturepath")
                        or lower_key in {"backgroundpic", "picturepath"}
                        or "pic" in lower_key
                    ) and value:
                        refs.add(value.replace("\\", "/"))
                else:
                    walk(value)
        elif isinstance(obj, list):
            for item in obj:
                walk(item)

    for ftu in sorted((ROOT / "ui").glob("*.ftu")):
        data = decode_ftu(ftu)[0]
        walk(data)

    for source_root in (ROOT / "src", ROOT / "tools"):
        if not source_root.exists():
            continue
        for path in source_root.rglob("*"):
            if path.suffix.lower() not in {".c", ".cc", ".cpp", ".h", ".py"}:
                continue
            try:
                add_string_literals(path.read_text(encoding="utf-8", errors="ignore"))
            except Exception:
                continue

    chars = set("".join(texts))
    chars.update("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")
    chars.update(" :/.-_年月日时分秒周星期上下左右确定取消返回开关成功失败修改地址源目标类型请更换请用新地址核对强制")
    chars.update("，。！？：；（）【】《》、·“”‘’");
    return "".join(sorted(chars)), refs


def subset_font(chars: str, dst: Path) -> None:
    options = subset.Options()
    options.layout_features = ["*"]
    options.name_IDs = ["*"]
    options.name_legacy = True
    options.name_languages = ["*"]
    options.notdef_outline = True
    options.recalc_average_width = True
    font = subset.load_font(str(SOURCE_FONT), options)
    subsetter = subset.Subsetter(options=options)
    subsetter.populate(text=chars)
    subsetter.subset(font)
    subset.save_font(font, str(dst), options)


def write_easyui_cfg() -> None:
    cfg = {
        "baud": "115200",
        "rotateTouch": 0,
        "rotateScreen": 0,
        "startupLibPath": "/res/lib/libzkgui.so",
        "languageCode": "zh_CN",
        "defBrightness": -1,
        "screensaverTimeOut": -1,
        "touchDev": "/dev/input/event1",
        "languagePath": "/res/tr/",
        "uart": "ttyS0",
        "startupTouchCalib": False,
        "zkdebug": False,
        "font": f"/res/font/{FONT_NAME}",
        "resPath": "/res/ui/",
    }
    (RES_ROOT / "etc").mkdir(parents=True, exist_ok=True)
    (RES_ROOT / "etc" / "EasyUI.cfg").write_text(
        json.dumps(cfg, ensure_ascii=False, indent=4), encoding="utf-8"
    )


def prepare_res_root(refs: set[str], chars: str) -> None:
    if RES_ROOT.exists():
        shutil.rmtree(RES_ROOT)
    for name in ["bin", "etc", "font", "lib", "tr", "ui"]:
        (RES_ROOT / name).mkdir(parents=True, exist_ok=True)

    shutil.copy2(ROOT / "Release" / "libzkgui.so", RES_ROOT / "lib" / "libzkgui.so")
    copy_tree(ROOT / "ui", RES_ROOT / "ui")

    for ref in refs:
        ui_src = ROOT / "ui" / ref
        res_src = ROOT / "resources" / ref
        dst = RES_ROOT / "ui" / ref
        if not dst.exists() and res_src.exists():
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(res_src, dst)

    # Keep the bundled input-method dictionary only if the current UI tree has it.
    if (ROOT / "ui" / "ime").exists():
        copy_tree(ROOT / "ui" / "ime", RES_ROOT / "ui" / "ime")
    elif (ROOT / "resources" / "ime").exists():
        copy_tree(ROOT / "resources" / "ime", RES_ROOT / "ui" / "ime")

    subset_font(chars, RES_ROOT / "font" / FONT_NAME)

    tr_dir = RES_ROOT / "tr"
    (tr_dir / "zh_CN-简体中文.json").write_text("{}\n", encoding="utf-8")
    (tr_dir / "zh_TW-繁體中文.json").write_text("{}\n", encoding="utf-8")
    (tr_dir / "en_US-ENGLISH.json").write_text("{}\n", encoding="utf-8")
    write_easyui_cfg()


def size_tree(path: Path) -> int:
    return sum(f.stat().st_size for f in path.rglob("*") if f.is_file())


def build_images() -> None:
    if not MK_SQUASHFS.exists() or not FSIMG.exists():
        raise FileNotFoundError("FlyThings image tools were not found")
    OUT_ROOT.mkdir(parents=True, exist_ok=True)
    run([
        str(MK_SQUASHFS),
        str(RES_ROOT),
        str(OUTPUT_SQFS),
        "-noappend",
        "-all-root",
        "-comp",
        "xz",
    ])
    if OUTPUT_SQFS.stat().st_size > RES_PARTITION_SIZE:
        raise RuntimeError(
            f"res.sqfs is too large: {OUTPUT_SQFS.stat().st_size} > {RES_PARTITION_SIZE}"
        )
    run([str(FSIMG), "-i", f"res:{OUTPUT_SQFS}", "-p", "t113", "-o", str(OUTPUT_IMG)])
    return None


def main() -> int:
    chars, refs = collect_ftu_text_and_refs()
    prepare_res_root(refs, chars)
    build_images()
    print(f"output_dir={OUT_ROOT}")
    print(f"res_root_size={size_tree(RES_ROOT)}")
    print(f"subset_font_size={(RES_ROOT / 'font' / FONT_NAME).stat().st_size}")
    print(f"res_sqfs_size={OUTPUT_SQFS.stat().st_size}")
    print(f"res_partition_size={RES_PARTITION_SIZE}")
    print(f"update_img={OUTPUT_IMG} size={OUTPUT_IMG.stat().st_size}")
    print(f"output_update={OUTPUT_IMG} size={OUTPUT_IMG.stat().st_size}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
