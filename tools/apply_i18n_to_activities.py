#!/usr/bin/env python3
from pathlib import Path


ACTIVITY_FILES = [
    ("deviceListActivity.cpp", "deviceList.ftu"),
    ("ethernetsettingActivity.cpp", "ethernetsetting.ftu"),
    ("lte4gsettingActivity.cpp", "lte4gsetting.ftu"),
    ("mainActivity.cpp", "main.ftu"),
    ("page1topsetActivity.cpp", "page1topset.ftu"),
    ("setdisplayActivity.cpp", "setdisplay.ftu"),
    ("showsysdateActivity.cpp", "showsysdate.ftu"),
    ("UserImeActivity.cpp", "UserIme.ftu"),
    ("wifisettingActivity.cpp", "wifisetting.ftu"),
]


def patch_file(filename: str, ftu: str) -> None:
    path = Path("src/activity") / filename
    text = path.read_text(encoding="utf-8", errors="ignore")
    header_name = filename.replace(".cpp", ".h")
    if '#include "logic/Cj96I18n.h"' not in text:
        lines = text.splitlines()
        for index, line in enumerate(lines):
            if line.startswith("#include ") and header_name in line:
                lines.insert(index + 1, '#include "logic/Cj96I18n.h"')
                text = "\n".join(lines) + "\n"
                break

    apply_line = f'    Cj96I18n::applyToActivity(this, "{ftu}");'
    if apply_line not in text:
        if "\tonUI_init();" in text:
            text = text.replace("\tonUI_init();", "\tonUI_init();\n" + apply_line, 1)
        elif "    onUI_init();" in text:
            text = text.replace("    onUI_init();", "    onUI_init();\n" + apply_line, 1)

    if text.count(apply_line) < 2:
        if "\tActivity::onResume();" in text:
            text = text.replace("\tActivity::onResume();", "\tActivity::onResume();\n" + apply_line, 1)
        elif "    Activity::onResume();" in text:
            text = text.replace("    Activity::onResume();", "    Activity::onResume();\n" + apply_line, 1)

    path.write_text(text, encoding="utf-8")
    print(f"patched {path} {ftu} apply_count={text.count(apply_line)}")


def main() -> None:
    for filename, ftu in ACTIVITY_FILES:
        patch_file(filename, ftu)


if __name__ == "__main__":
    main()
