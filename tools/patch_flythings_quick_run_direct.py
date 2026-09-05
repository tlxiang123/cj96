#!/usr/bin/env python3
"""Make FlyThingsIDE Ctrl+Alt+R use adb.exe for file transfer."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
from zipfile import ZIP_DEFLATED, ZipFile


IDE_ROOT = Path(r"D:\Install\FlyThingsIDE\bin")
PLUGIN = IDE_ROOT / "plugins" / "com.zkswe.ide.editor_2.0.0.202608170922.jar"
BACKUP = PLUGIN.with_suffix(".jar.cj96_direct_backup")
SOURCE_NAME = "src/com/zksw/editor/handler/LaunchOverAdb.java"
CLASS_PREFIX = "com/zksw/editor/handler/LaunchOverAdb"

CALL_MARKER = "                pushFiles(device, allEntries, monitor, targetDir);"
CALL_REPLACEMENT = "                pushFilesDirect(device, allEntries, monitor, targetDir);"
PROJECT_MARKER = "        properties = FlyThings.loadExtraProperties(mProject);"
PROJECT_INSERT_OLD = """        if (\"cj96\".equals(mProject.getName())) {
            runCurrentProjectDeploy(mProject);
            return null;
        }

"""
PROJECT_INSERT = """        if (\"cj96\".equals(mProject.getName())) {
            runCurrentProjectDeployAsync(mProject);
            return null;
        }

"""
PROJECT_METHOD_MARKER = "    private FileFilter dynamicLibraryFilter = new FileFilter() {"
PROJECT_ASYNC_METHOD = r'''    private void runCurrentProjectDeployAsync(final IProject project) {
        final File script = new File(project.getLocation().toFile(),
                "tools/CJ96_OneClickDeploy.py");
        Thread worker = new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    if (!script.isFile()) {
                        throw new IOException(
                                "当前工程缺少 tools/CJ96_OneClickDeploy.py");
                    }
                    String python = "D:\\Python\\Python314\\python.exe";
                    if (!new File(python).isFile()) {
                        python = "python";
                    }
                    Process process = new ProcessBuilder(python,
                            script.getAbsolutePath(), "--no-wait")
                            .directory(project.getLocation().toFile())
                            .redirectErrorStream(true).start();
                    try (java.io.BufferedReader reader = new java.io.BufferedReader(
                            new java.io.InputStreamReader(process.getInputStream()))) {
                        String line;
                        while ((line = reader.readLine()) != null) {
                            System.out.println("[CJ96 Ctrl+Alt+R] " + line);
                        }
                    }
                    int exitCode = process.waitFor();
                    if (exitCode != 0) {
                        throw new IOException(
                                "Python 推送失败，返回码: " + exitCode);
                    }
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                    reportCurrentProjectDeployFailure(e);
                } catch (Exception e) {
                    reportCurrentProjectDeployFailure(e);
                }
            }
        }, "CJ96-Ctrl-Alt-R-deploy");
        worker.setDaemon(true);
        worker.start();
    }

    private void reportCurrentProjectDeployFailure(final Exception error) {
        final String message = error.getMessage() == null
                ? error.toString() : error.getMessage();
        Display.getDefault().asyncExec(new Runnable() {
            @Override
            public void run() {
                Dialogs.openError(Messages.Error, "当前工程推送失败: " + message);
            }
        });
    }

'''
PROJECT_METHOD = r'''    private void runCurrentProjectDeploy(IProject project)
            throws ExecutionException {
        File script = new File(project.getLocation().toFile(),
                "tools/CJ96_OneClickDeploy.py");
        if (!script.isFile()) {
            throw new ExecutionException("当前工程缺少 tools/CJ96_OneClickDeploy.py");
        }
        String python = "D:\\Python\\Python314\\python.exe";
        if (!new File(python).isFile()) {
            python = "python";
        }
        try {
            Process process = new ProcessBuilder(python,
                    script.getAbsolutePath(), "--no-wait")
                    .directory(project.getLocation().toFile())
                    .redirectErrorStream(true).start();
            try (java.io.BufferedReader reader = new java.io.BufferedReader(
                    new java.io.InputStreamReader(process.getInputStream()))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    System.out.println("[CJ96 Ctrl+Alt+R] " + line);
                }
            }
            int exitCode = process.waitFor();
            if (exitCode != 0) {
                throw new ExecutionException(
                        "当前工程推送失败，Python 返回码: " + exitCode);
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new ExecutionException("当前工程推送被中断", e);
        } catch (IOException e) {
            throw new ExecutionException("无法启动当前工程推送脚本", e);
        }
    }

'''
METHOD_MARKER = "    private void pushFiles(JadbDevice device,"
METHOD = r'''    private void pushFilesDirect(JadbDevice device,
            Collection<SyncFileEntry> entries, IProgressMonitor progressMonitor,
            String extraTaskname) throws IOException, JadbException {
        final String adb = Util.getAdbAbsolutePath();
        final String serial = device.getSerial();
        final long totalBytes = entries.stream()
                .mapToLong(entry -> entry.getLocal().length()).sum();
        final String taskName = String.format("推送到 %s %s    共%d个文件%s",
                serial, extraTaskname, entries.size(), Util.sizeString(totalBytes));
        progressMonitor.beginTask(taskName,
                (int) Math.min(totalBytes, Integer.MAX_VALUE));

        for (SyncFileEntry entry : entries) {
            if (progressMonitor.isCanceled()) {
                return;
            }
            File local = entry.getLocal();
            String remote = ((RemoteFileRecord) entry.getRemote()).getPath();
            String remoteParent = remote.substring(0, remote.lastIndexOf('/'));
            runAdbDirect(adb, serial, "shell", "mkdir", "-p", remoteParent);
            progressMonitor.subTask(String.format("%s      %s/%s",
                    local.getName(), Util.sizeString(0),
                    Util.sizeString(local.length())));
            runAdbDirect(adb, serial, "push", local.getAbsolutePath(), remote);
            progressMonitor.worked((int) Math.min(local.length(), Integer.MAX_VALUE));
        }
        progressMonitor.done();
    }

    private void runAdbDirect(String adb, String serial, String... arguments)
            throws IOException, JadbException {
        List<String> command = new ArrayList<>();
        command.add(adb);
        command.add("-s");
        command.add(serial);
        for (String argument : arguments) {
            command.add(argument);
        }
        Process process = new ProcessBuilder(command)
                .redirectErrorStream(true).start();
        StringBuilder output = new StringBuilder();
        try (java.io.BufferedReader reader = new java.io.BufferedReader(
                new java.io.InputStreamReader(process.getInputStream()))) {
            String line;
            while ((line = reader.readLine()) != null) {
                output.append(line).append('\n');
            }
        }
        try {
            if (process.waitFor() != 0) {
                throw new JadbException("adb command failed: " + output.toString().trim());
            }
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            throw new IOException("adb command interrupted", e);
        }
    }

'''


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def restore() -> None:
    if not BACKUP.exists():
        raise FileNotFoundError(f"backup not found: {BACKUP}")
    shutil.copy2(BACKUP, PLUGIN)
    print(f"restored: {PLUGIN}")


def prepare_source(workdir: Path) -> Path:
    with ZipFile(PLUGIN) as archive:
        source = archive.read(SOURCE_NAME).decode("utf-8").replace("\r\n", "\n")
        source = source.replace(PROJECT_INSERT_OLD, "", 1)
        if CALL_REPLACEMENT not in source:
            if CALL_MARKER not in source:
                raise RuntimeError("Quick Run push call insertion point was not found")
            source = source.replace(CALL_MARKER, CALL_REPLACEMENT, 1)
        if PROJECT_INSERT not in source:
            if PROJECT_MARKER not in source:
                raise RuntimeError("Quick Run project insertion point was not found")
            source = source.replace(PROJECT_MARKER, PROJECT_INSERT + PROJECT_MARKER, 1)
        if PROJECT_ASYNC_METHOD not in source:
            source = source.replace(PROJECT_METHOD_MARKER,
                                    PROJECT_ASYNC_METHOD + PROJECT_METHOD_MARKER, 1)
        if PROJECT_METHOD not in source:
            if PROJECT_METHOD_MARKER not in source:
                raise RuntimeError("Quick Run project method insertion point was not found")
            source = source.replace(PROJECT_METHOD_MARKER, PROJECT_METHOD + PROJECT_METHOD_MARKER, 1)
        if METHOD not in source:
            if METHOD_MARKER not in source:
                raise RuntimeError("Quick Run method insertion point was not found")
            source = source.replace(METHOD_MARKER, METHOD + METHOD_MARKER, 1)

        source_file = workdir / SOURCE_NAME
        source_file.parent.mkdir(parents=True, exist_ok=True)
        source_file.write_text(source, encoding="utf-8")
        for name in archive.namelist():
            if name.startswith("lib/") and name.endswith(".jar"):
                target = workdir / "embedded_libs" / Path(name).name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(archive.read(name))
    return source_file


def compile_source(workdir: Path, source_file: Path) -> Path:
    classpath = ";".join([
        str(IDE_ROOT / "plugins" / "*"),
        str(workdir / "embedded_libs" / "*"),
    ])
    classes = workdir / "classes"
    classes.mkdir()
    dom4j_stub = workdir / "src" / "org" / "dom4j" / "DocumentException.java"
    dom4j_stub.parent.mkdir(parents=True, exist_ok=True)
    dom4j_stub.write_text(
        "package org.dom4j; public class DocumentException extends Exception {}\n",
        encoding="utf-8",
    )
    command = [
        "javac", "--release", "8", "-encoding", "UTF-8", "-classpath", classpath,
        "-d", str(classes), str(source_file), str(dom4j_stub),
    ]
    result = subprocess.run(command, capture_output=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(
            "javac failed:\n"
            + result.stdout.decode("gbk", errors="replace")
            + result.stderr.decode("gbk", errors="replace")
        )
    if not any(classes.rglob("LaunchOverAdb*.class")):
        raise RuntimeError("javac did not produce LaunchOverAdb classes")
    return classes


def replace_plugin(classes: Path) -> None:
    replacement = {
        path.relative_to(classes).as_posix(): path.read_bytes()
        for path in classes.rglob("LaunchOverAdb*.class")
    }
    with tempfile.NamedTemporaryFile(
            prefix="flythings_editor_", suffix=".jar", dir=PLUGIN.parent,
            delete=False) as handle:
        temporary = Path(handle.name)
    try:
        with ZipFile(PLUGIN) as source, ZipFile(temporary, "w", ZIP_DEFLATED) as output:
            for item in source.infolist():
                if item.filename in replacement:
                    output.writestr(item, replacement.pop(item.filename))
                else:
                    output.writestr(item, source.read(item.filename))
            for name, payload in replacement.items():
                output.writestr(name, payload)
        shutil.copy2(temporary, PLUGIN)
    finally:
        temporary.unlink(missing_ok=True)


def patch() -> None:
    if not PLUGIN.exists():
        raise FileNotFoundError(PLUGIN)
    if not BACKUP.exists():
        shutil.copy2(PLUGIN, BACKUP)
        print(f"backup: {BACKUP}")
    with tempfile.TemporaryDirectory(prefix="cj96_quick_run_direct_") as temporary:
        workdir = Path(temporary)
        classes = compile_source(workdir, prepare_source(workdir))
        replace_plugin(classes)
    with ZipFile(PLUGIN) as archive:
        class_bytes = archive.read(f"{CLASS_PREFIX}.class")
    if b"pushFilesDirect" not in class_bytes or b"runAdbDirect" not in class_bytes:
        raise RuntimeError("direct adb Quick Run patch verification failed")
    print(f"patched: {PLUGIN}")
    print(f"sha256: {sha256(PLUGIN)}")
    print("Restart FlyThingsIDE once before using Ctrl+Alt+R.")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--restore", action="store_true")
    args = parser.parse_args()
    if args.restore:
        restore()
    else:
        patch()


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"Quick Run direct adb patch error: {exc}", file=sys.stderr)
        raise SystemExit(1)
