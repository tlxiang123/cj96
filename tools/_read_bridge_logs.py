import subprocess
from pathlib import Path

adb = r"D:\Install\AndroidPlatformTools\adb.exe"
serial = "192.168.1.70:5555"
destination = Path("backups/bridge_logs_after_deploy")
destination.mkdir(parents=True, exist_ok=True)

for name in ("stderr.log", "run.log", "stdout.log"):
    remote = f"/mnt/extsd/tuya_demo/cj96_tuya_demo.{name}"
    local = destination / name
    result = subprocess.run(
        [adb, "-s", serial, "pull", remote, str(local)],
        capture_output=True,
        text=True,
    )
    print(result.stdout.strip())
    print(f"--- {local} ---")
    print(local.read_text(encoding="utf-8", errors="replace")[-12000:])
