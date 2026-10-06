from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"
OUT = ROOT / "diagnostics" / "untranslated_candidates_full.txt"

TRANSLATORS = (
    "translateRuntimeText", "translateDeviceName", "translateDeviceType",
    "translateStatusText", "formatValveGroupText",
)
INTERNAL_TRANSLATORS = (
    "setWindow5TestAddressTipWithColor", "setWindow5TestAddressTip",
    "setWindow5TestAddressSuccessTip", "setWindow5TestAddressFailureTip",
    "setWindow5ConfigTip", "page6ShowCycleTip", "showW3TipWindow",
    "setW3TipText", "appendValveGroupOperationLog", "setMainWifiTipText",
)
NON_DISPLAY = (
    "strcmp", "strncmp", "strstr", "strchr", "strncpy", "strcpy",
    "copyText", "snprintf", "sprintf", "strcasecmp", "printf",
    "LOGD", "LOGI", "LOGE", "LOGW", "access", "unlink", "fopen",
)
STRING_LITERAL = re.compile(r"\"((?:\\.|[^\"\\\\])*)\"")


def decode_hex_escapes(text: str) -> str:
    out = bytearray()
    i = 0
    while i < len(text):
        if text[i] == "\\" and i + 3 < len(text) and text[i + 1] in "xX":
            try:
                out.append(int(text[i + 2:i + 4], 16))
                i += 4
                continue
            except ValueError:
                pass
        out.extend(text[i].encode("utf-8"))
        i += 1
    return out.decode("utf-8", "replace")


def has_cjk(text: str) -> bool:
    return any(0x2E80 <= ord(ch) <= 0x9FFF or 0xFF00 <= ord(ch) <= 0xFFEF for ch in text)


def enclosing_call(source: str, position: int) -> str:
    depth = 0
    i = position - 1
    while i >= 0:
        ch = source[i]
        if ch == ")":
            depth += 1
        elif ch == "(":
            if depth == 0:
                head = source[:i].rstrip()
                m = re.search(r"([A-Za-z_][A-Za-z0-9_:]*)$", head)
                return m.group(1) if m else ""
            depth -= 1
        elif ch in ";{}" and depth == 0:
            return ""
        i -= 1
    return ""


def main() -> None:
    rows = []
    total = 0
    wrapped = 0
    for path in sorted(SRC.rglob("*.cc")):
        raw = path.read_text(encoding="utf-8", errors="replace")
        for m in STRING_LITERAL.finditer(raw):
            text = decode_hex_escapes(m.group(1))
            if not has_cjk(text):
                continue
            total += 1
            call = enclosing_call(raw, m.start())
            leaf = call.split("::")[-1]
            if leaf in TRANSLATORS or leaf in INTERNAL_TRANSLATORS:
                wrapped += 1
                continue
            if leaf in NON_DISPLAY or call.startswith("std::"):
                kind = "non-display"
            else:
                kind = "review"
            line_no = raw.count(chr(10), 0, m.start()) + 1
            line = raw.splitlines()[line_no - 1].strip()
            rel = path.relative_to(ROOT).as_posix()
            rows.append("%s:%d | %s | call=%s | %s | %s" % (rel, line_no, text, call or "None", kind, line))
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(chr(10).join(rows) + chr(10), encoding="utf-8")
    print("cjk=%d wrapped=%d candidates=%d" % (total, wrapped, len(rows)))
    print(OUT)


if __name__ == "__main__":
    main()
