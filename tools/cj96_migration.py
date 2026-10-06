#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# Offline migration helper. Does not build, deploy, contact ADB or change IDE.
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from datetime import datetime
import zipfile

ROOT = Path(__file__).resolve().parents[1]
REQUIRED = {
    'FlyThings IDE': 'D:/Install/FlyThingsIDE',
    'T113 make': 'D:/Install/FlyThingsIDE/sdk/toolchains/t113/bin/make.exe',
    'T113 compiler': 'D:/Install/FlyThingsIDE/sdk/toolchains/t113/bin/arm-unknown-linux-musleabihf-gcc.exe',
    'FlyThings dependency cache': 'D:/Install/FlyThingsIDE/bin/.dep',
    'Python expected by makefile.init': 'D:/Python/Python314/python.exe',
    'Go expected by build_project.py': 'D:/Install/Go/bin/go.exe',
    'Tuya SDK': 'D:/code/tuya/tuya-iot-core-sdk',
}
OPTIONAL = {
    'ADB': 'D:/Install/AndroidPlatformTools/adb.EXE',
    'AC firmware source': 'D:/code/cj96_client_ac',
    'DC firmware source': 'D:/code/cj96_client_dc',
    'Keil': 'D:/Keil_v5',
    'Tuya APP': 'C:/Users/Administrator/TuYaMiniProject/miniapp',
    'NodeJS': 'D:/Install/NodeJS',
    'Developer docs': 'C:/Users/Administrator/Documents/Codex/2026-06-23/d/outputs/flythings_developer_docs_local.md',
}
MANIFEST = 'CJ96_MIGRATION_MANIFEST.json'

def git_value(args):
    try:
        result = subprocess.run(['git', '-C', str(ROOT), *args], capture_output=True,
                                timeout=20, encoding='utf-8', errors='replace')
        if result.returncode:
            return {'error': result.stderr.strip()}
        return result.stdout.strip()
    except (OSError, subprocess.TimeoutExpired) as error:
        return {'error': str(error)}

def report():
    version_file = ROOT / 'src/FirmwareVersion.h'
    version = 'unknown'
    if version_file.exists():
        for line in version_file.read_text(encoding='utf-8').splitlines():
            if line.startswith('#define CJ96_FIRMWARE_VERSION '):
                version = line.split(maxsplit=2)[2].strip(chr(34))
    state = git_value(['status', '--porcelain'])
    return {
        'created_at': datetime.now().astimezone().isoformat(),
        'root': str(ROOT),
        'python_executable': sys.executable,
        'python_version': sys.version,
        'source_firmware_version': version,
        'git_head': git_value(['rev-parse', 'HEAD']),
        'git_branch': git_value(['branch', '--show-current']),
        'git_changed_entries': len(state.splitlines()) if isinstance(state, str) else state,
        'required': {k: {'path': v, 'exists': Path(v).exists()} for k, v in REQUIRED.items()},
        'optional': {k: {'path': v, 'exists': Path(v).exists()} for k, v in OPTIONAL.items()},
        'note': 'Presence checks only; not a build, IDE/plugin, deployment or board verification.',
    }

def linked(path):
    return path.is_symlink() or getattr(path, 'is_junction', lambda: False)()

def collect_files():
    # Refuse links instead of silently omitting externally referenced data.
    result = []
    def fail(error):
        raise error
    for base, directories, files in os.walk(ROOT, onerror=fail, followlinks=False):
        for name in directories + files:
            path = Path(base) / name
            if linked(path):
                raise RuntimeError('Linked path requires manual migration: ' + str(path))
        for name in files:
            path = Path(base) / name
            if not path.is_file():
                raise RuntimeError('Not a regular file: ' + str(path))
            result.append(path)
    return sorted(result)

def stamp(path):
    stat = path.stat()
    return (stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns)

def pack(destination):
    destination = destination.expanduser().resolve()
    if destination.is_relative_to(ROOT):
        raise ValueError('ZIP must be outside the project directory.')
    if destination.suffix.lower() != '.zip':
        raise ValueError('Output must end with .zip')
    partial = destination.with_suffix(destination.suffix + '.partial')
    if destination.exists() or partial.exists():
        raise FileExistsError('Output or .partial already exists. Choose a new name.')
    if (ROOT / '.git').is_file():
        raise RuntimeError('Git worktree uses external metadata; migrate that metadata separately first.')
    inventory = report()
    files = collect_files()
    stamps = {p: stamp(p) for p in files}
    total = sum(value[0] for value in stamps.values())
    print('Files:', len(files), 'Bytes:', total)
    print('WARNING: full private project, potentially including credentials/logs. No encryption.')
    print('External IDE/SDK/other projects are NOT included.')
    destination.parent.mkdir(parents=True, exist_ok=True)
    entries = []
    with zipfile.ZipFile(partial, 'x', compression=zipfile.ZIP_DEFLATED,
                         compresslevel=1, allowZip64=True) as archive:
        for index, path in enumerate(files, 1):
            before = stamps[path]
            if linked(path) or stamp(path) != before:
                raise RuntimeError('Source changed before packing: ' + str(path))
            name = 'cj96/' + path.relative_to(ROOT).as_posix()
            digest = hashlib.sha256()
            size = 0
            info = zipfile.ZipInfo.from_file(path, arcname=name, strict_timestamps=False)
            info.compress_type = zipfile.ZIP_DEFLATED
            with path.open('rb') as source, archive.open(info, 'w', force_zip64=True) as target:
                while chunk := source.read(1024 * 1024):
                    digest.update(chunk)
                    size += len(chunk)
                    target.write(chunk)
            if stamp(path) != before or size != before[0]:
                raise RuntimeError('Source changed during packing: ' + str(path))
            entries.append({'path': name, 'size': size, 'sha256': digest.hexdigest()})
            if index % 500 == 0:
                print('Packed', index, '/', len(files), flush=True)
        if files != collect_files() or any(stamp(p) != stamps[p] for p in files):
            raise RuntimeError('Project changed during packing. Close writers and try again.')
        archive.writestr(MANIFEST, json.dumps({
            'format': 1, 'scope': 'project only; external dependencies excluded',
            'environment': inventory, 'files': entries,
        }, ensure_ascii=False, indent=2))
    verify(partial)
    if destination.exists():
        raise FileExistsError('Destination appeared during packing; not replacing it.')
    partial.rename(destination)
    print('Complete:', destination)

def verify(path):
    with zipfile.ZipFile(path, 'r') as archive:
        names = archive.namelist()
        if len(names) != len(set(names)):
            raise ValueError('Duplicate ZIP entries')
        manifest = json.loads(archive.read(MANIFEST))
        if manifest.get('format') != 1:
            raise ValueError('Unsupported manifest format')
        expected = manifest['files']
        expected_names = [item['path'] for item in expected]
        if len(expected_names) != len(set(expected_names)):
            raise ValueError('Duplicate manifest entries')
        if set(names) != set(expected_names) | {MANIFEST}:
            raise ValueError('Archive entries do not match manifest')
        for item in expected:
            digest = hashlib.sha256()
            size = 0
            with archive.open(item['path']) as source:
                while chunk := source.read(1024 * 1024):
                    digest.update(chunk)
                    size += len(chunk)
            if size != item['size'] or digest.hexdigest() != item['sha256']:
                raise ValueError('Checksum mismatch: ' + item['path'])
    print('Verified SHA-256:', len(expected), 'files; archive not extracted or executed.')

def main():
    parser = argparse.ArgumentParser(description='Offline CJ96 migration check/project pack/verify. No deployment.')
    group = parser.add_mutually_exclusive_group()
    group.add_argument('--pack', type=Path, metavar='OUTSIDE_PROJECT_ZIP')
    group.add_argument('--verify', type=Path, metavar='ZIP')
    parser.add_argument('--report', type=Path, metavar='JSON')
    args = parser.parse_args()
    if args.verify:
        verify(args.verify)
        return 0
    data = report()
    print(json.dumps(data, ensure_ascii=False, indent=2))
    if args.report:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        with args.report.open('x', encoding='utf-8') as output:
            json.dump(data, output, ensure_ascii=False, indent=2)
    if args.pack:
        pack(args.pack)
    return 0

if __name__ == '__main__':
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
    try:
        raise SystemExit(main())
    except (OSError, ValueError, RuntimeError, KeyError, zipfile.BadZipFile) as error:
        print('ERROR: ' + str(error), file=sys.stderr)
        print('No deployment performed. Incomplete .partial files are not migration packages.', file=sys.stderr)
        raise SystemExit(1)
