#!/usr/bin/env python3
"""Generate a grouped multilingual overflow report from FTU layouts."""

from __future__ import annotations

import csv
import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
FONT = ROOT / "font" / "Alibaba-PuHuiTi-Regular.ttf"
OUTPUT = ROOT / "i18n" / "preview_i18n_overflow.png"
COLUMNS = ["English", "Deutsch", "Français", "日本語"]


def controls(node):
    if isinstance(node, dict):
        if node.get("id") and "position" in node:
            yield node
        for value in node.values():
            yield from controls(value)
    elif isinstance(node, list):
        for value in node:
            yield from controls(value)


def load_translations():
    with (ROOT / "i18n" / "ftu_texts_multi_language.csv").open(encoding="utf-8-sig", newline="") as h:
        rows = list(csv.DictReader(h))
    return {(r["所在FTU"], str(r["控件ID"])): r for r in rows if r.get("字段名") == "text"}


def main():
    import sys
    sys.path.insert(0, str(ROOT / "tools"))
    from ftu_style import decode_ftu
    translations = load_translations()
    grouped = {}
    probe = ImageDraw.Draw(Image.new("RGB", (8, 8)))
    for ftu in sorted((ROOT / "ui").glob("*.ftu")):
        data, _, _ = decode_ftu(ftu)
        for control in controls(data):
            row = translations.get((ftu.name, str(control.get("id", ""))))
            if not row:
                continue
            for column in COLUMNS:
                text = row.get(column, "")
                if not text:
                    continue
                font_size = int(control.get("fontSize", 20) or 20)
                font = ImageFont.truetype(str(FONT), font_size)
                width = int(control["position"].get("width", 0))
                actual = probe.textbbox((0, 0), text, font=font)[2]
                if actual <= width:
                    continue
                ratio = actual / max(width, 1)
                grouped.setdefault((ftu.name, row["中文原文"]), []).append((column, text, width, actual, ratio))
    rows = []
    for (ftu, source), values in grouped.items():
        max_ratio = max(v[-1] for v in values)
        shortest = min(values, key=lambda value: len(value[1]))[1]
        suggestion = shortest if max_ratio >= 1.7 else ("缩字号/改宽" if max_ratio >= 1.3 else "微调")
        rows.append((max_ratio, ftu, source, shortest, suggestion, values))
    rows.sort(reverse=True)
    title_font = ImageFont.truetype(str(FONT), 28)
    text_font = ImageFont.truetype(str(FONT), 22)
    row_height = 54
    image = Image.new("RGB", (1500, 90 + row_height * len(rows)), "white")
    draw = ImageDraw.Draw(image)
    draw.text((24, 18), f"多语言溢出汇总：{len(rows)} 条源文案；红=严重超出，黄=可缩字号/改宽", font=title_font, fill="black")
    headers = ["最差超出", "页面", "中文原文", "推荐简写", "处理建议", "详情"]
    xs = [24, 180, 340, 620, 920, 1120]
    for x, label in zip(xs, headers):
        draw.text((x, 66), label, font=text_font, fill="#333333")
    y = 112
    for max_ratio, ftu, source, shortest, suggestion, values in rows:
        color = "#D93025" if max_ratio >= 1.7 else "#E37400"
        detail = f"{len(values)}处，最大 {values[0][0]} {values[0][3]}/{values[0][2]}px"
        for x, value in zip(xs, [f"{max_ratio:.2f}x", ftu, source, shortest, suggestion, detail]):
            draw.text((x, y), value, font=text_font, fill=color)
        y += row_height
    OUTPUT.write_bytes(b"") if False else image.save(OUTPUT, optimize=True)
    print(f"grouped={len(rows)}")
    print(f"output={OUTPUT}")
    for row in rows[:100]:
        print(f"{row[0]:.2f}x\t{row[1]}\t{row[2]}\t=> {row[3]}\t{row[4]}\t{row[5]}")


if __name__ == "__main__":
    main()
