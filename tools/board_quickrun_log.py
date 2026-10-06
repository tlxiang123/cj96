#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CJ96 board-side quick-run log capture; the IDE shortcut is untouched."""

import argparse
from datetime import datetime
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
from uuid import uuid4

ADB = os.environ.get('CJ96_ADB', r'D:\Install\AndroidPlatformTools\adb.EXE')
DEVICE = os.environ.get('CJ96_DEVICE', '192.168.1.69:5555')
GCC = Path('D:/Install/FlyThingsIDE/sdk/toolchains/t113/bin/arm-unknown-linux-musleabihf-gcc.exe')
LAUNCHER_SOURCE = Path(__file__).with_name('quickrun_logger_daemon.c')
ROOT = '/mnt/extsd/cj96_quickrun_logs'
ACTIVE = ROOT + '/active_session.txt'
SESSION_RE = re.compile(r'^' + re.escape(ROOT) + r'/quickrun_\d{8}_\d{6}_[0-9a-f]{6}$')
ROTATE_KIB = 1024
ROTATED_FILES = 15  # Including the active file, approximately 16 MiB maximum.


def adb(*args, timeout=15, check=True):
    result = subprocess.run([ADB, '-s', DEVICE, *args], capture_output=True, timeout=timeout)
    if check and result.returncode:
        message = (result.stdout + result.stderr).decode('utf-8', 'replace').strip()
        raise RuntimeError(f'ADB failed ({result.returncode}): {message}')
    return result


def text(*args, timeout=15, check=True):
    return adb(*args, timeout=timeout, check=check).stdout.decode('utf-8', 'replace').strip()


def connected():
    if text('get-state', timeout=6) != 'device':
        raise RuntimeError(f'{DEVICE} is not connected')


def session_path():
    path = text('shell', 'cat', ACTIVE, check=False).strip()
    return path if SESSION_RE.fullmatch(path) else None


def push_text(content, destination):
    fd, name = tempfile.mkstemp(prefix='cj96_diag_', suffix='.txt')
    try:
        with os.fdopen(fd, 'w', encoding='utf-8', newline='\n') as output:
            output.write(content)
        adb('push', name, destination, timeout=20)
    finally:
        Path(name).unlink(missing_ok=True)


def read_pid(path):
    raw = text('shell', 'cat', path + '/logcat.pid', check=False)
    return int(raw) if raw.isdecimal() else None


def logger_running(path):
    pid = read_pid(path)
    if not pid:
        return False
    proc = adb('shell', 'cat', f'/proc/{pid}/cmdline', check=False).stdout
    args = proc.replace(b'\r', b'').split(b'\0')
    return any(arg.endswith(b'/logcat') or arg == b'logcat' for arg in args) and (
        (path + '/logcat.txt').encode('ascii') in args
    )


def start():
    connected()
    current = session_path()
    if current and logger_running(current):
        print('Already capturing on board: ' + current)
        return
    path = ROOT + '/quickrun_' + datetime.now().strftime('%Y%m%d_%H%M%S') + '_' + uuid4().hex[:6]
    adb('shell', 'mkdir', '-p', path)
    snapshots = [f'Host time: {datetime.now().astimezone().isoformat()}', f'Device: {DEVICE}']
    for title, command in (
        ('OS version', ('getprop', 'ro.build.version.release')),
        ('Kernel', ('cat', '/proc/version')),
        ('Uptime', ('cat', '/proc/uptime')),
        ('Free storage', ('df', '/mnt/extsd')),
        ('Mounts', ('mount',)),
        ('Processes before reproduction', ('ps',)),
    ):
        snapshots.append('\n[' + title + ']\n' + text('shell', *command, check=False))
    push_text('\n'.join(snapshots) + '\n', path + '/board_info.txt')
    logfile = path + '/logcat.txt'
    # Python builds a tiny standalone launcher for this non-Python board.
    # It creates a new session before exec'ing the stock board-side logcat.
    with tempfile.TemporaryDirectory(prefix='cj96_logger_build_') as tmp:
        binary = Path(tmp) / 'quickrun_logger_daemon'
        build = subprocess.run(
            [str(GCC), '-std=c99', '-D_DEFAULT_SOURCE', '-static', '-Os', '-s',
             '-Wall', '-Wextra', str(LAUNCHER_SOURCE), '-o', str(binary)],
            capture_output=True, timeout=60,
        )
        if build.returncode:
            raise RuntimeError('Diagnostic launcher build failed: ' +
                               (build.stdout + build.stderr).decode('utf-8', 'replace'))
        adb('push', str(binary), ROOT + '/logger_daemon', timeout=20)
    output = text('shell', ROOT + '/logger_daemon', logfile, timeout=10)
    match = re.search(r'\b(\d+)\s*$', output)
    if not match:
        raise RuntimeError('No logcat PID returned: ' + repr(output))
    push_text(match.group(1) + '\n', path + '/logcat.pid')
    time.sleep(1.5)
    if not logger_running(path):
        raise RuntimeError('Board logcat is not running; DO NOT reproduce yet: ' + path)
    listing = text('shell', 'ls', '-l', logfile, check=False)
    if 'No such file' in listing or not listing:
        raise RuntimeError('Log file was not created; DO NOT reproduce yet: ' + path)
    push_text(path + '\n', ACTIVE)
    print('Board-side capture is running, PID=' + match.group(1))
    print('Log directory: ' + path)
    print('Log file: ' + logfile + ' (rotated: .1 to .15)')


def status():
    connected()
    path = session_path()
    if not path:
        print('No active session recorded on the board.')
        return
    print('Log directory: ' + path)
    print('Capture: ' + ('RUNNING' if logger_running(path) else 'STOPPED; files remain on board'))
    print(text('shell', 'ls', '-l', path, check=False))
    print('Board uptime: ' + text('shell', 'cat', '/proc/uptime', check=False))


def stop():
    connected()
    path = session_path()
    if not path or not logger_running(path):
        print('No capture running; previous logs are kept.')
        return
    pid = read_pid(path)
    adb('shell', 'kill', str(pid))
    print(f'Stopped logcat PID={pid}; files remain at {path}')


def pull():
    connected()
    path = session_path()
    if not path:
        raise RuntimeError('No board-side session found')
    local = Path(__file__).resolve().parent.parent / 'diagnostics' / Path(path).name
    local.parent.mkdir(parents=True, exist_ok=True)
    adb('pull', path, str(local), timeout=120)
    print('Copied for analysis: ' + str(local))
    print('Board-side original remains at: ' + path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=('start', 'status', 'stop', 'pull'))
    args = parser.parse_args()
    try:
        globals()[args.action]()
    except (RuntimeError, subprocess.TimeoutExpired, OSError) as exc:
        print('Error: ' + str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
