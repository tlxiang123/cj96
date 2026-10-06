from pathlib import Path
import re

for path in Path("src/activity").glob("*.cpp"):
    text = path.read_text(encoding="utf-8", errors="ignore")
    updated = re.sub(
        r'Cj96I18n::applyToActivity\(this,\s*("[^"]+")\);',
        r'CJ96_I18N_APPLY(\1);',
        text,
    )
    if updated != text:
        path.write_text(updated, encoding="utf-8")
        print(f"patched {path}")
