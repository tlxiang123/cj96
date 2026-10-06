from pathlib import Path
import base64, io, json, re, struct, subprocess, zlib
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]
PANELS = [Path('C:/Users/Administrator/TuYaMiniProject/miniapp'), ROOT/'integrations/tuya/panel']
NODE = 'D:/Install/NodeJS/node.exe'
JS = r"""
const fs = require('fs'), path = require('path'), assert = require('assert');
const root = process.argv[1], panel = process.argv[2];
const ts = require(path.join(root, 'node_modules/typescript'));
const source = fs.readFileSync(path.join(panel, 'src/pages/home/index.tsx'), 'utf8');
const a = source.indexOf('        {showW2GroupChoice && (');
const b = source.indexOf('        {showW2GroupBind && (', a);
assert(a > 0 && b > a);
const section = source.slice(a, b);
const jsx = section.slice(section.indexOf('<View'), section.lastIndexOf('</View>') + 7);
const React = {createElement: (type, props, ...children) => ({type, props: {...props, children: children.flat(Infinity)}})};
const styles = new Proxy({}, {get: (_, key) => key});
const View = 'View', Image = 'Image', Text = 'Text', Input = 'Input';
const w2PlainButtonBackground = 'native-background';
const clamp = (n, min, max) => Math.max(min, Math.min(max, n));
let w2GroupChoiceMode = 'bind', w2GroupChoiceNo = 2, calls = [];
const setW2GroupChoiceNo = n => {w2GroupChoiceNo = n;};
const stepW2GroupChoice = n => calls.push(['step', n]);
const confirmW2GroupChoiceAll = () => calls.push('all');
const confirmW2GroupChoiceCurrent = () => calls.push('confirm');
const closeW2GroupChoice = () => calls.push('cancel');
const compiled = ts.transpileModule('(' + jsx + ')', {compilerOptions: {jsx: ts.JsxEmit.React, target: ts.ScriptTarget.ES2020}}).outputText;
const render = () => eval(compiled);
function all(node) {return node && typeof node === 'object' ? [node, ...node.props.children.flatMap(all)] : [];}
function find(tree, cls) {return all(tree).filter(n => (n.props.className || '').split(' ').includes(cls));}
function text(node) {return node == null || typeof node === 'boolean' ? '' : typeof node === 'object' ? node.props.children.map(text).join('') : String(node);}
const previews = [];
for (const mode of ['bind', 'clear', 'delete', 'rename']) {
  w2GroupChoiceMode = mode; w2GroupChoiceNo = mode === 'delete' ? 72 : 2; calls = [];
  const tree = render(), hasAll = mode === 'bind' || mode === 'clear';
  const labels = ['确认', '取消'];
  assert.strictEqual(find(tree, 'w2GroupChoiceAll').length, hasAll ? 1 : 0);
  for (const [cls, label] of [['Confirm', '确认'], ['Cancel', '取消'], ...(hasAll ? [['All', '全部']] : [])]) {
    const button = find(tree, 'w2GroupChoice' + cls)[0];
    assert.strictEqual(text(button), label);
    const children = button.props.children.filter(Boolean);
    assert.strictEqual(children.length, 2);
    assert.strictEqual(children[0].type, Image);
    assert.strictEqual(children[0].props.src, w2PlainButtonBackground);
    assert.strictEqual(children[1].type, Text);
    button.props.onClick();
  }
  assert.deepStrictEqual(calls, ['confirm', 'cancel', ...(hasAll ? ['all'] : [])]);
  find(tree, 'w2GroupChoicePrev')[0].props.onClick();
  find(tree, 'w2GroupChoiceNext')[0].props.onClick();
  assert.deepStrictEqual(calls.slice(-2), [['step', -1], ['step', 1]]);
  const input = find(tree, 'w2GroupChoiceNumberInput')[0];
  for (const [value, expected] of [['72',72],['0',1],['999',128],['bad',1]]) {
    input.props.onInput({detail:{value}});
    assert.strictEqual(w2GroupChoiceNo, expected);
  }
  previews.push({mode, tree});
}
if (process.argv[3]) fs.writeFileSync(process.argv[3], JSON.stringify(previews));
console.log('PASS: 4 popup modes; separate native background and text; confirm/cancel/all callbacks; arrows and bounded input');
"""

def rule(css, selector):
    matches = re.findall(re.escape('.'+selector) + r'\s*\{([^}]+)\}', css)
    assert matches, selector
    return dict(pair for match in matches for pair in re.findall(r'([a-z-]+):\s*([^;]+);', match))

def main():
    out = ROOT/'diagnostics/app_group_choice_buttons_20261006'
    out.mkdir(parents=True, exist_ok=True)
    blob = (ROOT/'ui/main.ftu').read_bytes()
    native = json.loads(zlib.decompress(blob[struct.unpack_from('<I',blob,6)[0]+8:-6]))['window__229']['window__284']
    report = []
    for i, panel in enumerate(PANELS):
        css = (panel/'src/pages/home/index.module.less').read_text(encoding='utf-8')
        background = panel/'src/assets/cj96/confirm_cancel_bg_120x60.png'
        assert background.read_bytes() == (ROOT/'resources/confirm_cancel_bg_120x60.png').read_bytes()
        for key in ['button__285','button__286','button__291']:
            assert native[key]['backgroundPic'] == background.name
            assert native[key]['fontSize'] == 20
        label = rule(css, 'w2GroupChoiceButtonLabel')
        assert label['font-weight'] == '400'
        assert label['font-size'] == 'min(1.953125vw, 3.333333333vh)'
        for cls in ['w2GroupChoiceButtonLabel','w2GroupChoiceButtonBackground']:
            assert rule(css, cls)['pointer-events'] == 'none'
        assert float(rule(css,'w2GroupChoiceCancel')['left'].rstrip('%')) < float(rule(css,'w2GroupChoiceConfirm')['left'].rstrip('%'))
        font_css = (panel/'src/pages/home/device-font.less').read_text(encoding='utf-8')
        font = TTFont(io.BytesIO(base64.b64decode(re.search(r'base64,([^)]*)',font_css).group(1))))
        assert all(ord(c) in font.getBestCmap() for c in '确认取消全部')
        run = subprocess.run([NODE,'-e',JS,str(PANELS[0]),str(panel),str(out/('trees_'+str(i)+'.json'))],capture_output=True,text=True,encoding='utf-8')
        assert run.returncode == 0, run.stdout + run.stderr
        print(panel, run.stdout.strip())
        report.append({'panel':str(panel),'checks':run.stdout.strip(),'native_style':'120x60 transparent outline, separate 20px normal-weight text, cancel left / confirm right','phone_tested':False})
    (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')

if __name__ == '__main__':
    main()
