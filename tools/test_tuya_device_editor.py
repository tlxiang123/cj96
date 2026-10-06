from pathlib import Path
import base64, io, json, re, struct, subprocess, zlib
from PIL import Image
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]
PANELS = [Path(r'C:/Users/Administrator/TuYaMiniProject/miniapp'), ROOT/'integrations/tuya/panel']
NODE = r'D:/Install/NodeJS/node.exe'
ASSETS = ['w2set_region_1.png', 'w2set_region_2.png', 'w2set_region_3.png', 'w2_change_irr_list_546x184.png', 'w2_set_bind_113x113_norm.png', 'w2_set_clear_113x113_norm.png', 'w2_set_delete_group_113x113_norm.png', 'w2_set_rename_group_113x113_norm.png', 'w2_address_pin_fixed_50x50.png', 'w2_device_name_fixed_icon_50x50.png', 'w2_irrnum_fixed_icon_113x69.png', 'w2_single_digit_frame_27x39.png', 'confirm_cancel_bg_120x60.png']
JS = r"""
const fs = require('fs'), path = require('path'), assert = require('assert');
const ts = require(path.join(process.argv[1], 'node_modules/typescript'));
const source = fs.readFileSync(path.join(process.argv[2], 'src/pages/home/index.tsx'), 'utf8');
const a = source.indexOf('        {showW2Editor && (');
const b = source.indexOf('        {showW2Capacity && (', a);
const section = source.slice(a, b);
assert(a > 0 && b > a);
const jsx = section.slice(section.indexOf('<View'), section.lastIndexOf('</View>') + 7);
const styles = new Proxy({}, {get: (_, key) => key});
const React = {createElement: (type, props, ...children) => ({type, props: {...props, children: children.flat(Infinity)}})};
const View = 'View', Text = 'Text', Image = 'Image', ScrollView = 'ScrollView';
const clamp = (n, min, max) => Math.min(max, Math.max(min, n));
let w2EditingAddress = '239', w2EditingName = '电磁阀239', w2SelectedGroupNo = 72;
let w2EditingIndex = 225, w2GroupPreviewEnabled = false, w2EditingTypeIndex = 0;
let w2GroupNumbers = Array.from({length: 72}, (_, i) => i + 1), devices = [];
let selectedName = '', calls = [];
const setW2SelectedGroupNo = value => {w2SelectedGroupNo = value;};
const setW2GroupPreviewEnabled = value => {w2GroupPreviewEnabled = value;};
const setW2GroupName = value => {selectedName = value;};
const formatPanelGroupSummary = group => '电磁阀[' + [17 + group*3, 18 + group*3, 19 + group*3].join(', ') + ']';
const openW2Capacity = () => calls.push('capacity');
const cancelW2Editor = () => calls.push('cancel');
const saveW2Editor = () => calls.push('save');
const openW2GroupBind = () => calls.push('bind');
const openW2ClearTip = () => calls.push('clear');
const openW2GroupChoice = mode => calls.push(mode);
const openW2Rename = () => calls.push('rename');
const openW2DeviceRename = () => calls.push('deviceRename');
const names = ['w2SetRegion1','w2SetRegion2','w2SetRegion3','w2AddressCombinedImage','w2DeviceNameLabelImage','w2DigitFrame','w2ChangeIrrListBackground','w2PlainButtonBackground','w2BindGroupImage','w2ClearGroupImage','w2DeleteGroupImage','w2RenameGroupImage','w2IrrLabelImage'];
const declarations = names.map(name => 'const ' + name + ' = ' + JSON.stringify(name) + ';').join('\n');
const compiled = ts.transpileModule(declarations + '\n const render = () => (' + jsx + '); render();', {compilerOptions: {jsx: ts.JsxEmit.React, target: ts.ScriptTarget.ES2020}}).outputText;
function render() {return eval(compiled);}
function all(node) {return node && typeof node === 'object' ? [node, ...node.props.children.flatMap(all)] : [];}
function find(tree, cls) {return all(tree).filter(node => (node.props.className || '').split(' ').includes(cls));}
function text(node) {return node == null ? '' : typeof node === 'object' ? node.props.children.map(text).join('') : String(node);}
let tree = render();
assert.strictEqual(text(find(tree, 'w2EditorAddressDigits')[0]), '239');
assert.strictEqual(text(find(tree, 'w2EditorGroupDigits')[0]), '072');
assert.strictEqual(text(find(tree, 'w2EditorNameValue')[0]), '电磁阀239');
assert.strictEqual(find(tree, 'w2EditorDigitFrame').length, 6);
assert.strictEqual(find(tree, 'w2EditorListRows')[0].type, ScrollView);
assert.strictEqual(find(tree, 'w2EditorListRows')[0].props.scrollY, true);
assert.strictEqual(find(tree, 'w2EditorListRow').length, 72);
assert.strictEqual(text(find(tree, 'w2EditorListRow')[71]), '阀组[72] 电磁阀[233, 234, 235]');
assert.deepStrictEqual(find(tree, 'w2EditorListButton').map(text), ['一键添加', '取消', '确认']);
assert.deepStrictEqual(find(tree, 'w2EditorTool').map(text), ['关联传感器', '清空阀组', '删除阀组', '修改名称']);
for (const button of [...find(tree, 'w2EditorListButton'), ...find(tree, 'w2EditorTool'), ...find(tree, 'w2EditorNameValue')]) button.props.onClick();
assert.deepStrictEqual(calls, ['capacity','cancel','save','bind','clear','delete','rename','deviceRename']);
find(tree, 'w2EditorListRow')[3].props.onClick();
assert.strictEqual(w2SelectedGroupNo, 4);
assert.strictEqual(w2GroupPreviewEnabled, true);
assert.strictEqual(selectedName, '阀组[4]');
assert.strictEqual(text(find(render(), 'w2EditorGroupDigits')[0]), '004');
w2EditingAddress = '20';
assert.strictEqual(text(find(render(), 'w2EditorAddressDigits')[0]), '020');
for (const count of [0,4,14,72,128]) {
  w2GroupNumbers = Array.from({length: count}, (_, i) => i + 1);
  tree = render();
  assert.strictEqual(find(tree, 'w2EditorListRow').length, count);
  assert.strictEqual(find(tree, 'w2EditorListButton').length, 3);
  assert.strictEqual(find(tree, 'w2EditorListRows')[0].props.children[0].props.className, 'w2EditorListContent');
}
console.log('PASS: dynamic values, 0/4/14/72/128 groups, ScrollView, independent labels, 8 preserved handlers, selection and leading zeros');
"""

def rules(css, selector):
    m = re.search(re.escape('.'+selector) + r' \{([^}]+)\}', css)
    assert m, selector
    return dict(re.findall(r'([a-z-]+):\s*([^;]+);', m.group(1)))

def rect(css, selector, parent):
    r = rules(css, selector)
    return [float(r[k].rstrip('%'))*parent[i%2]/100 for i,k in enumerate(['left','top','width','height'])]

def expected(node):
    return [node['position'][k] for k in ['left','top','width','height']]

def equal(actual, target):
    assert all(abs(a-b)<.001 for a,b in zip(actual,target)), (actual,target)

def main():
    blob=(ROOT/'ui/main.ftu').read_bytes()
    ftu=json.loads(zlib.decompress(blob[struct.unpack_from('<I',blob,6)[0]+8:-6]))
    w=ftu['window__229']; r1=w['window__230']; r2=w['window__239']; r3=w['window__245']
    report=[]
    for panel in PANELS:
        css=(panel/'src/pages/home/index.module.less').read_text(encoding='utf-8')
        equal(rect(css,'w2EditorDialog',(1024,600)),expected(w))
        for i,r in enumerate([r1,r2,r3],1): equal(rect(css,'w2EditorRegion'+str(i),(1007,400)),expected(r))
        for selector,key in [('AddressImage','textview__235'),('AddressLabel','textview__232'),('NameIcon','textview__238'),('NameLabel','textview__233'),('NameValue','edittext__234')]:
            equal(rect(css,'w2Editor'+selector,(590,88)),expected(r1[key]))
        for parent,parent_size,keys in [('w2EditorAddressDigits',(590,88),['edittext__231','edittext__236','edittext__237']),('w2EditorGroupDigits',(201,69),['edittext__252','edittext__251','edittext__253'])]:
            container=rect(css,parent,parent_size)
            if parent=='w2EditorGroupDigits':
                header=rect(css,'w2EditorGroupHeader',(383,373))
                container[0]+=header[0]; container[1]+=header[1]
            digit=rules(css,'w2EditorDigit')
            for i,key in enumerate(keys,1):
                x=float(rules(css,'w2EditorDigit:nth-child('+str(i)+')')['left'].rstrip('%'))*container[2]/100
                width=float(digit['width'].rstrip('%'))*container[2]/100
                equal([container[0]+x,container[1],width,container[3]],expected((r1 if parent=='w2EditorAddressDigits' else r3)[key]))
        equal(rect(css,'w2EditorList',(590,279)),expected(r2['listview__240']))
        assert rules(css,'w2EditorList')['overflow']=='hidden'
        assert rules(css,'w2EditorListRow')['height']=='min(4.00390625vw, 6.833333333vh)'
        assert rules(css,'w2EditorListRow')['margin-bottom']=='min(0.48828125vw, 0.833333333vh)'
        assert 'grid-template-rows' not in rules(css,'w2EditorListRows')
        buttons=rect(css,'w2EditorListButtons',(590,279))
        for i,key in enumerate(['button__244','button__243','button__242'],1):
            x=float(rules(css,'w2EditorListButton:nth-child('+str(i)+')')['left'].rstrip('%'))*590/100
            equal([x,buttons[1],120,buttons[3]],expected(r2[key]))
        for kind,icon_key,label_key in [('Bind','button__248','textview__255'),('Clear','button__247','textview__256'),('Delete','button__249','textview__257'),('Rename','button__250','textview__258')]:
            outer=rect(css,'w2EditorTool'+kind,(383,373))
            for cls,key in [('w2EditorToolImage',icon_key),('w2EditorToolLabel',label_key)]:
                inner=rect(css,'w2EditorTool'+kind+' .'+cls,outer[2:])
                equal([outer[0]+inner[0],outer[1]+inner[1],inner[2],inner[3]],expected(r3[key]))
        for selector in ['w2EditorAddressDigits .w2EditorDigitText', 'w2EditorGroupDigits .w2EditorDigitText']:
            assert rules(css, selector)['font-weight'] == '700'
        assert rules(css, 'w2EditorListRowSelected')['background'] == '#d9eeff'
        asset_report=[]
        for name in ASSETS:
            p=panel/'src/assets/cj96'/name
            assert p.read_bytes()==(ROOT/'resources'/name).read_bytes(),name
            image=Image.open(p).convert('RGBA')
            asset_report.append({'name':name,'canvas':image.size,'visible_alpha_bounds':image.getbbox()})
        font_css=(panel/'src/pages/home/device-font.less').read_text()
        font=TTFont(io.BytesIO(base64.b64decode(re.search(r'base64,([^)]*)',font_css).group(1))))
        assert all(ord(c) in font.getBestCmap() for c in '设备地址设备名称已选阀组关联传感器清空阀组删除阀组修改名称一键添加取消确认0123456789')
        run=subprocess.run([NODE,'-e',JS,str(PANELS[0]),str(panel)],capture_output=True,text=True,encoding='utf-8')
        assert run.returncode==0,run.stdout+run.stderr
        print(panel,run.stdout.strip())
        report.append({'panel':str(panel),'geometry':'native FTU controls match within 0.001px','assets':asset_report,'handlers':run.stdout.strip(),'phone_render':'not tested'})
    out=ROOT/'diagnostics/app_device_editor_20261006'
    out.mkdir(parents=True,exist_ok=True)
    (out/'verification.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print('PASS')

if __name__=='__main__':
    main()
