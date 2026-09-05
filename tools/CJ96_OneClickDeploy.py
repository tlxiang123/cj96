#!/usr/bin/env python3
"""Build and directly deploy the complete current CJ96 board UI."""

import argparse
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[1]


def run(script: str) -> None:
    subprocess.run([sys.executable, script], cwd=ROOT, check=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--no-wait", action="store_true")
    args = parser.parse_args()
    try:
        print("[1/2] Building CJ96 UI...", flush=True)
        run("tools/build_project.py")
        print("[2/2] Directly deploying UI, resources, font, and library...", flush=True)
        run("tools/deploy_current_to_zkgui.py")
        print("Deployment complete. The board UI service has been restarted.", flush=True)
        return 0
    except subprocess.CalledProcessError as error:
        print(f"Deployment failed with exit code {error.returncode}.", flush=True)
        return error.returncode or 1
    finally:
        if not args.no_wait:
            try:
                input("Press Enter to close this window...")
            except EOFError:
                pass


if __name__ == "__main__":
    raise SystemExit(main())
