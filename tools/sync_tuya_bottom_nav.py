from pathlib import Path
from datetime import datetime
import base64, io, json, re, shutil
from fontTools import subset
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]
PANELS = [Path(r'C:/Users/Administrator/TuYaMiniProject/miniapp'), ROOT / 'integrations/tuya/panel']
PAGES = ['overview', 'device', 'plan', 'test', 'log']
LABELS = ['总览', '设备', '计划', '测试', '日志']
NAMES = ['bgr_btn1', 'bgr_btn2', 'bgr_btn3', 'bgr_btn4', 'bgr_btn_log']


def main():
    backup = ROOT / 'backups' / ('tuya_bottom_nav_' + datetime.now().strftime('%Y%m%d_%H%M%S'))
    font = TTFont(ROOT / 'font/Alibaba-PuHuiTi-Regular.ttf')
    sub = subset.Subsetter(options=subset.Options())
    sub.populate(text=''.join(LABELS))
    sub.subset(font)
    font.flavor = 'woff2'
    buffer = io.BytesIO()
    font.save(buffer)
    encoded = base64.b64encode(buffer.getvalue()).decode('ascii')
    font_css = '@font-face {\n  font-family: CJ96NavLabels;\n  src: url(data:font/woff2;base64,' + encoded + ') format(' + chr(39) + 'woff2' + chr(39) + ');\n  font-style: normal;\n  font-weight: 400;\n  font-display: swap;\n}\n'
    css = (ROOT / 'tools/tuya_bottom_nav.less').read_text(encoding='utf-8')
    for index, panel in enumerate(PANELS):
        tsx = panel / 'src/pages/home/index.tsx'
        less = panel / 'src/pages/home/index.module.less'
        old_tsx = tsx.read_text(encoding='utf-8')
        old_less = less.read_text(encoding='utf-8')
        pattern = r'        <View className=\{styles.bottomNav\}>.*?        </View>'
        match = re.search(pattern, old_tsx, re.S)
        assert match, panel
        old_lines = match.group().splitlines()
        assert len(old_lines) == 7, 'Expected original five image buttons; do not patch twice'
        new_lines = [old_lines[0], '          {/* Local FTU: independent icon and label, with a shared click target. */}']
        for page,label,line in zip(PAGES,LABELS,old_lines[1:-1]):
            event = re.search(r' onClick=\{(.*?)\} />(.*)$', line)
            assert event, line
            image = line[:event.start()] + ' />'
            image = image.replace('styles.navItem', 'styles.navIcon')
            cls = 'navItem' + page.title()
            new_lines.append('          <View className={styles.navItem + '+chr(39)+' '+chr(39)+' + styles.'+cls+'} onClick={'+event.group(1)+'}>')
            new_lines.append('  ' + image)
            new_lines.append('            <Text className={styles.navLabel}>' + label + '</Text>')
            new_lines.append('          </View>')
        new_lines.append(old_lines[-1])
        new_tsx = old_tsx[:match.start()] + '\n'.join(new_lines) + old_tsx[match.end():]
        new_less,n = re.subn(r'\.bottomNav \{.*?(?=\.modalOverlay \{)', lambda _:css+'\n',old_less,count=1,flags=re.S)
        assert n == 1
        new_less = '@import '+chr(39)+'./nav-font.less'+chr(39)+';\n\n' + new_less
        changes = [(tsx,new_tsx.encode('utf-8')),(less,new_less.encode('utf-8')),(less.with_name('nav-font.less'),font_css.encode('utf-8'))]
        for name in NAMES:
            for suffix in ['', '_ch']:
                filename = name+suffix+'.png'
                changes.append((panel/'src/assets/cj96'/filename,(ROOT/'resources'/filename).read_bytes()))
        for path,data in changes:
            if path.exists():
                saved = backup/str(index)/path.relative_to(panel)
                saved.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(path,saved)
            path.write_bytes(data)
        print('Updated:',panel)
    (backup/'report.json').write_text(json.dumps({'panels':[str(p) for p in PANELS],'font_subset_bytes':len(buffer.getvalue())},indent=2),encoding='utf-8')
    print('Backup:',backup)
    print('Font subset bytes:',len(buffer.getvalue()))


if __name__ == '__main__':
    main()
