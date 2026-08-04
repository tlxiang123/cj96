#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


GENERATED_ACTIVITY_FILES = [
    Path("src/activity/ethernetsettingActivity.cpp"),
    Path("src/activity/lte4gsettingActivity.cpp"),
    Path("src/activity/page1topsetActivity.cpp"),
    Path("src/activity/setdisplayActivity.cpp"),
    Path("src/activity/showsysdateActivity.cpp"),
    Path("src/activity/softapsettingActivity.cpp"),
    Path("src/activity/UserImeActivity.cpp"),
    Path("src/activity/wifisettingActivity.cpp"),
]


def patch_generated_activity(path: Path) -> None:
    text = path.read_text(encoding="utf-8", errors="ignore")
    original = text

    if '#include "logic/globalScreenshotLogic.cc"' not in text:
        lines = text.splitlines()
        output: list[str] = []
        inserted = False
        for line in lines:
            output.append(line)
            if (
                not inserted
                and line.startswith('#include "logic/')
                and line.endswith('Logic.cc"')
            ):
                output.append('#include "logic/globalScreenshotLogic.cc"')
                inserted = True
        if not inserted:
            raise RuntimeError(f"logic include not found in {path}")
        text = "\n".join(output) + ("\n" if original.endswith("\n") else "")

    if "20120, onButtonClick_GlobalScreenshotButton" not in text:
        needle = "static S_ButtonCallback sButtonCallbackTab[] = {\n"
        if needle not in text:
            raise RuntimeError(f"callback table not found in {path}")
        text = text.replace(
            needle,
            needle + "    20120, onButtonClick_GlobalScreenshotButton,\n",
            1,
        )

    if text != original:
        path.write_text(text, encoding="utf-8", newline="")


def patch_device_list_activity() -> None:
    path = Path("src/activity/deviceListActivity.cpp")
    text = path.read_text(encoding="utf-8", errors="ignore")
    original = text

    if '#include "logic/globalScreenshotLogic.cc"' not in text:
        text = text.replace(
            '#include "entry/EasyUIContext.h"\n',
            '#include "entry/EasyUIContext.h"\n#include "logic/globalScreenshotLogic.cc"\n',
            1,
        )

    if "#define ID_DEVICELIST_GlobalScreenshotButton 20120" not in text:
        text = text.replace(
            "#define ID_DEVICELIST_StatusSubItem 24005\n",
            "#define ID_DEVICELIST_StatusSubItem 24005\n"
            "#define ID_DEVICELIST_GlobalScreenshotButton 20120\n",
            1,
        )

    if "case ID_DEVICELIST_GlobalScreenshotButton:" not in text:
        text = text.replace(
            "    switch (pBase->getID()) {\n",
            "    switch (pBase->getID()) {\n"
            "    case ID_DEVICELIST_GlobalScreenshotButton:\n"
            "        onButtonClick_GlobalScreenshotButton(static_cast<ZKButton*>(pBase));\n"
            "        return;\n",
            1,
        )

    if text != original:
        path.write_text(text, encoding="utf-8", newline="")


def main() -> None:
    for path in GENERATED_ACTIVITY_FILES:
        patch_generated_activity(path)
    patch_device_list_activity()
    print("patched global screenshot callbacks")


if __name__ == "__main__":
    main()
