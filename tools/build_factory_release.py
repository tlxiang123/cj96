#!/usr/bin/env python3
"""Build a CJ96 factory release package."""

from __future__ import annotations

import hashlib
import json
import shutil
import subprocess
import sys
import zipfile
from datetime import datetime
from pathlib import Path

from firmware_version import read_firmware_version


ROOT = Path(__file__).resolve().parents[1]
RELEASE = ROOT / "Release"


def run_python(script: Path, *args: str) -> None:
    subprocess.run([sys.executable, str(script), *args], cwd=ROOT, check=True)


def latest_update_image() -> Path:
    candidates = [
        path
        for path in RELEASE.glob("internal_update_*/update.img")
        if path.is_file()
    ]
    if not candidates:
        raise FileNotFoundError("Release/internal_update_*/update.img not found")
    return max(candidates, key=lambda path: (path.stat().st_mtime_ns, path.name))


def digest(path: Path) -> dict[str, object]:
    md5 = hashlib.md5()
    sha256 = hashlib.sha256()
    size = 0
    with path.open("rb") as file:
        for block in iter(lambda: file.read(1024 * 1024), b""):
            size += len(block)
            md5.update(block)
            sha256.update(block)
    return {
        "path": str(path),
        "size": size,
        "md5": md5.hexdigest(),
        "sha256": sha256.hexdigest(),
    }


def write_factory_readme(path: Path, version: str) -> None:
    path.write_text(
        "\n".join(
            [
                f"CJ96 产线发布包 {version}",
                "",
                "目录说明：",
                "1. program/update.img：中科世为/FlyThings 本地升级镜像。",
                f"2. tuya_ota/update_{version}.bin：涂鸦后台上传用 OTA 文件，字节内容与 update.img 一致。",
                "3. sdcard_template/cj96_data：TF 卡持久化数据目录模板。",
                "4. manifest.json：文件大小、MD5、SHA256 和版本记录。",
                "",
                "产线注意：",
                "1. 烧录或 OTA 更新程序时，不要删除 /mnt/extsd/cj96_data。",
                "2. 用户配置保存在 /mnt/extsd/cj96_data/config。",
                "3. 用户日志保存在 /mnt/extsd/cj96_data/logs。",
                "4. /mnt/extsd/cj96_tuya_demo 是 OTA 运行目录，可以清理残留 update.img，但不要当作用户配置目录。",
                "",
            ]
        ),
        encoding="utf-8",
    )


def factory_layout_text() -> str:
    return "\n".join(
        [
            "CJ96 factory TF-card layout",
            "program files: /mnt/extsd/EasyUI.cfg /mnt/extsd/lib /mnt/extsd/ui /mnt/extsd/resources /mnt/extsd/font",
            "persistent config: /mnt/extsd/cj96_data/config",
            "persistent logs: /mnt/extsd/cj96_data/logs",
            "OTA runtime: /mnt/extsd/cj96_tuya_demo",
            "",
        ]
    )


def write_factory_archive(output_dir: Path) -> Path:
    archive_path = output_dir.parent / f"{output_dir.name}.zip"
    with zipfile.ZipFile(
        archive_path, "w", compression=zipfile.ZIP_DEFLATED
    ) as archive:
        for path in sorted(output_dir.rglob("*")):
            if path.is_file():
                archive.write(path, path.relative_to(output_dir.parent))
    return archive_path


def main() -> int:
    version = read_firmware_version()
    stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_dir = RELEASE / "factory" / f"cj96_factory_{version}_{stamp}"
    program_dir = output_dir / "program"
    ota_dir = output_dir / "tuya_ota"
    sd_template = output_dir / "sdcard_template"

    run_python(ROOT / "tools" / "prepare_tuya_bridge.py")
    run_python(ROOT / "tools" / "build_project.py")
    run_python(ROOT / "tools" / "build_internal_update_image.py")

    source_img = latest_update_image()
    update_img = program_dir / "update.img"
    ota_bin = ota_dir / f"update_{version}.bin"
    program_dir.mkdir(parents=True, exist_ok=True)
    ota_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source_img, update_img)
    run_python(
        ROOT / "tools" / "prepare_tuya_ota_bin.py",
        "--source",
        str(source_img),
        "--output",
        str(ota_bin),
    )

    for directory in [
        sd_template / "cj96_data" / "config",
        sd_template / "cj96_data" / "logs",
        sd_template / "cj96_data" / "backup",
        sd_template / "cj96_tuya_demo" / "inbox",
        sd_template / "cj96_tuya_demo" / "ack",
    ]:
        directory.mkdir(parents=True, exist_ok=True)
        (directory / ".keep").write_text("", encoding="utf-8")
    (sd_template / "cj96_data" / "factory_layout.txt").write_text(
        factory_layout_text(), encoding="utf-8"
    )

    readme = output_dir / "README_factory.txt"
    write_factory_readme(readme, version)

    manifest = {
        "product": "CJ96",
        "firmware_version": version,
        "created_at": datetime.now().isoformat(timespec="seconds"),
        "source_update_img": str(source_img),
        "update_img": digest(update_img),
        "tuya_ota_bin": digest(ota_bin),
        "persistent_data_root": "/mnt/extsd/cj96_data",
        "persistent_config": "/mnt/extsd/cj96_data/config",
        "persistent_logs": "/mnt/extsd/cj96_data/logs",
        "ota_runtime": "/mnt/extsd/cj96_tuya_demo",
        "do_not_overwrite": ["/mnt/extsd/cj96_data"],
    }
    (output_dir / "manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )
    archive_path = write_factory_archive(output_dir)

    print(f"factory_release={output_dir}")
    print(f"version={version}")
    print(f"update_img={update_img}")
    print(f"tuya_ota_bin={ota_bin}")
    print(f"readme={readme}")
    print(f"archive={archive_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
