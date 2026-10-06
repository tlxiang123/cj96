from pathlib import Path
import base64, hashlib, io, json, re, struct, zlib
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]
PANELS = [Path(r'C:/Users/Administrator/TuYaMiniProject/miniapp'), ROOT/'integrations/tuya/panel']
PAGES = ['overview','device','plan','test','log']
LABELS = ['总览','设备','计划','测试','日志']
NAMES = ['bgr_btn1','bgr_btn2','bgr_btn3','bgr_btn4','bgr_btn_log']


def main():
    blob=(ROOT/'ui/main.ftu').read_bytes()
    ftu=json.loads(zlib.decompress(blob[struct.unpack_from('<I',blob,6)[0]+8:-6]))
    controls=[v for v in ftu.values() if isinstance(v,dict)]
    report=[]
    for panel in PANELS:
        source=(panel/'src/pages/home/index.tsx').read_text(encoding='utf-8')
        nav=source.split('<View className={styles.bottomNav}>',1)[1].split('{window2Opening',1)[0]
        css=(panel/'src/pages/home/index.module.less').read_text(encoding='utf-8')
        def rules(name):
            body=re.search(re.escape('.'+name)+r' \{([^}]+)\}',css).group(1)
            return dict(re.findall(r'([a-z-]+):\s*([^;]+);',body))
        def number(value):
            return float(value.rstrip('%'))/100
        parent=rules('bottomNav')
        top=600*number(parent['top'])
        height=600*number(parent['height'])
        target=rules('navItem')
        width=1024*number(target['width'])
        for page,label,name in zip(PAGES,LABELS,NAMES):
            assert '<Text className={styles.navLabel}>'+label+'</Text>' in nav
            handler='openWindow2' if page=='device' else '() => setActivePage('+chr(39)+page+chr(39)+')'
            assert 'styles.navItem'+page.title()+'} onClick={'+handler+'}' in nav
            node=next(v for v in controls if v.get('picTab',{}).get('pic0')==name+'.png')
            label_node=next(v for v in controls if v.get('text')==label and v.get('position',{}).get('top',0)>=490)
            x=1024*number(rules('navItem'+page.title())['left'])
            icon=rules('navIcon')
            if page=='plan': icon.update(rules('navItemPlan .navIcon'))
            actual=[x+width*number(icon['left']),top,width*number(icon['width']),height*number(icon['height'])]
            expected=[node['position'][k] for k in ['left','top','width','height']]
            assert all(abs(a-b)<0.001 for a,b in zip(actual,expected)),(page,actual,expected)
            lr=rules('navLabel')
            actual_label=[x,top+height*number(lr['top']),width,height*number(lr['height'])]
            expected_label=[label_node['position'][k] for k in ['left','top','width','height']]
            assert all(abs(a-b)<0.001 for a,b in zip(actual_label,expected_label))
            for suffix in ['','_ch']:
                filename=name+suffix+'.png'
                assert (panel/'src/assets/cj96'/filename).read_bytes()==(ROOT/'resources'/filename).read_bytes()
        font_css=(panel/'src/pages/home/nav-font.less').read_text(encoding='utf-8')
        encoded=re.search(r'base64,([^)]*)',font_css).group(1)
        font=TTFont(io.BytesIO(base64.b64decode(encoded)))
        assert all(ord(char) in font.getBestCmap() for char in ''.join(LABELS))
        assert nav.count('onClick=')==5
        report.append({'panel':str(panel),'geometry':'matches FTU within 0.001px','assets':'all ten byte-identical','labels':'five separate Text elements with original handlers','font':'all ten glyphs present'})
    out=ROOT/'diagnostics/app_nav_20261006'
    out.mkdir(parents=True,exist_ok=True)
    (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    # Layout illustration from the verified coordinates; not a phone screenshot.
    preview=Image.new('RGB',(1024,120),'white')
    draw=ImageDraw.Draw(preview)
    font=ImageFont.truetype(str(ROOT/'font/Alibaba-PuHuiTi-Regular.ttf'),22)
    for label,name in zip(LABELS,NAMES):
        node=next(v for v in controls if v.get('picTab',{}).get('pic0')==name+'.png')
        p=node['position']
        image=Image.open(ROOT/'resources'/(name+'.png')).convert('RGBA')
        preview.paste(image,(p['left'],p['top']-480),image)
        p=next(v for v in controls if v.get('text')==label and v.get('position',{}).get('top',0)>=490)['position']
        draw.text((p['left']+p['width']/2,p['top']-480+p['height']/2),label,font=font,fill='#005bbb',anchor='mm')
    preview.save(out/'bottom_nav_preview.png')
    print(json.dumps(report,ensure_ascii=False,indent=2))
    print('PASS')


if __name__=='__main__':
    main()
