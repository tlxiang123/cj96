from pathlib import Path
import re


FILES = [
    "src/logic/mainLogic.cc",
    "src/logic/page1topsetLogic.cc",
    "src/logic/page2Logic.cc",
    "src/logic/page3Logic.cc",
    "src/logic/page4Logic.cc",
    "src/logic/ethernetsettingLogic.cc",
    "src/logic/wifisettingLogic.cc",
]

CHINESE = re.compile(r'"[^"\n]*[\u4e00-\u9fff][^"\n]*"')
IGNORE = re.compile(r"translateRuntimeText|translateDeviceName|translateStatusText|formatValveGroupText")


def main():
    results = []
    for name in FILES:
        path = Path(name)
        if not path.exists():
            continue
        for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            stripped = line.strip()
            if stripped.startswith(("//", "*", "/*")):
                continue
            if "LOGD" in line or "LOGI" in line or "LOGE" in line:
                continue
            if CHINESE.search(line) and not IGNORE.search(line):
                results.append(f"{path.as_posix()}:{line_no}: {stripped}")
    report = Path("diagnostics/unwrapped_ui_text.txt")
    report.write_text("\n".join(results) + "\n", encoding="utf-8")
    print(f"{len(results)} suspicious UI strings")
    print("\n".join(results))


if __name__ == "__main__":
    main()
