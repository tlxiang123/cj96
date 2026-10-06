#!/usr/bin/env python3
"""Export Chinese text from FTU layouts for translation."""

from __future__ import annotations

import csv
import re
import zipfile
from pathlib import Path
from xml.sax.saxutils import escape

from ftu_style import decode_ftu


ROOT = Path(__file__).resolve().parents[1]
UI_DIR = ROOT / "ui"
I18N_DIR = ROOT / "i18n"
CSV_OUTPUT = I18N_DIR / "ftu_texts_zh_CN.csv"
XLSX_OUTPUT = I18N_DIR / "ftu_texts_zh_CN.xlsx"
TEXT_FIELDS = {"text", "hint", "hintText", "title", "label"}
CHINESE_RE = re.compile(r"[\u3400-\u9fff]")


def has_chinese(value: str) -> bool:
    return bool(CHINESE_RE.search(value))


def control_type_from_path(path: list[str]) -> str:
    for part in reversed(path):
        if "__" in part:
            return part.split("__", 1)[0]
    return ""


def make_key(ftu_name: str, control_id: object, caption: str, field: str, index: int) -> str:
    base = Path(ftu_name).stem
    if control_id not in (None, ""):
        return f"{base}.{control_id}.{field}"
    if caption:
        safe_caption = re.sub(r"[^0-9A-Za-z_\u3400-\u9fff]+", "_", caption).strip("_")
        return f"{base}.{safe_caption}.{field}"
    return f"{base}.text_{index:04d}.{field}"


def walk(node: object, ftu_name: str, path: list[str], rows: list[dict[str, str]]) -> None:
    if isinstance(node, dict):
        caption = str(node.get("caption", ""))
        control_id = node.get("id", "")
        control_type = control_type_from_path(path)
        for field in TEXT_FIELDS:
            value = node.get(field)
            if isinstance(value, str) and value.strip() and has_chinese(value):
                rows.append(
                    {
                        "key": make_key(ftu_name, control_id, caption, field, len(rows) + 1),
                        "中文原文": value,
                        "英文": "",
                        "所在FTU": ftu_name,
                        "控件ID": str(control_id),
                        "控件名/caption": caption,
                        "控件类型": control_type,
                        "字段名": field,
                        "JSON路径": ".".join(path),
                        "备注": "",
                    }
                )
        for key, value in node.items():
            walk(value, ftu_name, path + [str(key)], rows)
    elif isinstance(node, list):
        for index, value in enumerate(node):
            walk(value, ftu_name, path + [str(index)], rows)


def collect_rows() -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for ftu_path in sorted(UI_DIR.glob("*.ftu")):
        data, _header, _offset = decode_ftu(ftu_path)
        walk(data, ftu_path.name, [], rows)
    return rows


def write_csv(rows: list[dict[str, str]]) -> None:
    I18N_DIR.mkdir(parents=True, exist_ok=True)
    fieldnames = [
        "key",
        "中文原文",
        "英文",
        "所在FTU",
        "控件ID",
        "控件名/caption",
        "控件类型",
        "字段名",
        "JSON路径",
        "备注",
    ]
    with CSV_OUTPUT.open("w", encoding="utf-8-sig", newline="") as output:
        writer = csv.DictWriter(output, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def column_name(index: int) -> str:
    name = ""
    while index:
        index, remainder = divmod(index - 1, 26)
        name = chr(65 + remainder) + name
    return name


def sheet_xml(rows: list[dict[str, str]]) -> str:
    fieldnames = [
        "key",
        "中文原文",
        "英文",
        "所在FTU",
        "控件ID",
        "控件名/caption",
        "控件类型",
        "字段名",
        "JSON路径",
        "备注",
    ]
    sheet_rows = []
    all_rows = [dict(zip(fieldnames, fieldnames))] + rows
    for row_index, row in enumerate(all_rows, 1):
        cells = []
        for col_index, field in enumerate(fieldnames, 1):
            value = escape(str(row.get(field, "")))
            ref = f"{column_name(col_index)}{row_index}"
            cells.append(f'<c r="{ref}" t="inlineStr"><is><t>{value}</t></is></c>')
        sheet_rows.append(f'<row r="{row_index}">{"".join(cells)}</row>')
    column_widths = [28, 24, 24, 16, 12, 24, 12, 12, 42, 18]
    cols = "".join(
        f'<col min="{index}" max="{index}" width="{width}" customWidth="1"/>'
        for index, width in enumerate(column_widths, 1)
    )
    return (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<worksheet xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
        'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">'
        f"<cols>{cols}</cols>"
        f'<sheetData>{"".join(sheet_rows)}</sheetData>'
        "</worksheet>"
    )


def write_xlsx(rows: list[dict[str, str]]) -> None:
    content_types = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">'
        '<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>'
        '<Default Extension="xml" ContentType="application/xml"/>'
        '<Override PartName="/xl/workbook.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"/>'
        '<Override PartName="/xl/worksheets/sheet1.xml" ContentType="application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"/>'
        '<Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>'
        '<Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>'
        "</Types>"
    )
    rels = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="xl/workbook.xml"/>'
        "</Relationships>"
    )
    workbook = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<workbook xmlns="http://schemas.openxmlformats.org/spreadsheetml/2006/main" '
        'xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">'
        '<sheets><sheet name="FTU中文文本" sheetId="1" r:id="rId1"/></sheets>'
        "</workbook>"
    )
    workbook_rels = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">'
        '<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/worksheet" Target="worksheets/sheet1.xml"/>'
        "</Relationships>"
    )
    core = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties" '
        'xmlns:dc="http://purl.org/dc/elements/1.1/" '
        'xmlns:dcterms="http://purl.org/dc/terms/" '
        'xmlns:dcmitype="http://purl.org/dc/dcmitype/" '
        'xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">'
        "<dc:creator>Codex</dc:creator><dc:title>CJ96 FTU Chinese Texts</dc:title>"
        "</cp:coreProperties>"
    )
    app = (
        '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>'
        '<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties" '
        'xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">'
        "<Application>Codex Python Exporter</Application></Properties>"
    )
    with zipfile.ZipFile(XLSX_OUTPUT, "w", compression=zipfile.ZIP_DEFLATED) as xlsx:
        xlsx.writestr("[Content_Types].xml", content_types)
        xlsx.writestr("_rels/.rels", rels)
        xlsx.writestr("xl/workbook.xml", workbook)
        xlsx.writestr("xl/_rels/workbook.xml.rels", workbook_rels)
        xlsx.writestr("xl/worksheets/sheet1.xml", sheet_xml(rows))
        xlsx.writestr("docProps/core.xml", core)
        xlsx.writestr("docProps/app.xml", app)


def main() -> None:
    rows = collect_rows()
    write_csv(rows)
    write_xlsx(rows)
    print(f"rows={len(rows)}")
    print(f"csv={CSV_OUTPUT}")
    print(f"xlsx={XLSX_OUTPUT}")


if __name__ == "__main__":
    main()
