from pathlib import Path
import base64, io, json, re, struct, subprocess, zlib
from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]
PANELS = [Path(r'C:/Users/Administrator/TuYaMiniProject/miniapp'), ROOT / 'integrations/tuya/panel']
NODE = Path(r'D:/Install/NodeJS/node.exe')

# Execute the actual TSX device render functions with isolated platform state.
# This tests handlers and generated controls, not native phone rendering.
RUNNER = r"""
const fs = require('fs');
const path = require('path');
const assert = require('assert');
const active = process.argv[1];
const panel = process.argv[2];
const ts = require(path.join(active, 'node_modules/typescript'));
const source = fs.readFileSync(path.join(panel, 'src/pages/home/index.tsx'), 'utf8');
const start = source.indexOf('  const scrollDeviceListToBottom =');
const end = source.indexOf('  const renderPlanPage =', start);
assert(start > 0 && end > start);
const headers = source.match(/const DEVICE_LIST_HEADERS = ([^;]+);/)[0];
const test = headers + '\n' + source.slice(start, end) + '\n' + `
let page = renderDevicePage();
let header = page.props.children[1];
assert.strictEqual(header.props.children.length, 6);
assert.strictEqual(text(header.props.children[5]), '底部');
assert.strictEqual(header.props.children[0].props.onClick, undefined);
header.props.children[5].props.onClick();
assert.strictEqual(deviceListScrollTop, 1000000);
header.props.children[5].props.onClick();
assert.strictEqual(deviceListScrollTop, 1000001);
page = renderDevicePage();
assert.strictEqual(page.props.children[2].props.scrollTop, 1000001);
assert.strictEqual(page.props.children[2].props.hideScrollbar, true);
let rows = page.props.children[2].props.children[0].props.children;
assert.strictEqual(rows.length, 9);
assert.strictEqual(text(rows[6].props.children[0]), '20');
assert.strictEqual(text(rows[7].props.children[0]), '239');
let footer = rows[8];
assert.deepStrictEqual(footer.props.children.map(text), ['点击添加', '总线设备 2', '', '', '', '同步']);
footer.props.children[0].props.onClick();
assert.strictEqual(newAddress, '20');
assert.strictEqual(addShown, true);
footer.props.children[5].props.onClick();
assert.strictEqual(syncCalls, 1);
assert.strictEqual(footer.props.children[2].props.onClick, undefined);
assert.strictEqual(footer.props.children[4].props.onClick, undefined);
assert.strictEqual(devices.length, 8);
rows[6].props.children[5].props.onClick();
assert.strictEqual(deletedIndex, 7); // Sorted row preserves source index.
rows[6].props.children[0].props.onClick();
assert.strictEqual(editedIndex, 7);
assert.strictEqual(rows[0].props.children[5].props.onClick, undefined);
syncingDevices = true;
footer = renderDeviceRow({ kind: 'empty', index: devices.length });
assert.strictEqual(text(footer.props.children[5]), '同步中');
assert.strictEqual(footer.props.children[5].props.onClick, undefined);
assert.strictEqual(footer.props.children[2].props.onClick, undefined);
assert.strictEqual(footer.props.children[4].props.onClick, undefined);
syncingDevices = false;
for (const count of [0, 6, 7, 222, 242]) {
  devices = Array.from({length: count}, (_, i) => ({address: i, name: '设备', group: '-'}));
  footer = renderDeviceRow({ kind: 'empty', index: count });
  assert.strictEqual(text(footer.props.children[1]), '总线设备 ' + Math.max(0, count - 6));
}
console.log('PASS: headers, repeated bottom clicks, footer order/count, sorted row actions, sync lock, debug actions absent');
`;
const compiled = ts.transpileModule(test, { compilerOptions: { jsx: ts.JsxEmit.React, target: ts.ScriptTarget.ES2020 } }).outputText;
const React = { createElement: (type, props, ...children) => ({ type, props: {...props, children: children.flat(Infinity)} }) };
const styles = new Proxy({}, {get: (_, key) => key});
let devices = [...Array.from({length: 6}, (_, i) => ({address: i + 1, name: '默认设备', group: '-', connected: false})), {address: 239, name: '电磁阀239', group: '72', connected: true, status: '关闭'}, {address: 20, name: '电磁阀20', group: '1', connected: true, status: '关闭'}];
let deviceListScrollTop = 0, syncingDevices = false, sending = false, deviceReportPending = false;
let tip = '', tipShown = false, newAddress = '', addShown = false, syncCalls = 0, deletedIndex = -1, editedIndex = -1;
const View = 'View', Text = 'Text', Image = 'Image', ScrollView = 'ScrollView';
const DEFAULT_DEVICE_COUNT = 6, deviceListBackground = 'w2_bgr_v2.png';
const setDeviceListScrollTop = update => { deviceListScrollTop = update(deviceListScrollTop); };
const setW2TipText = value => { tip = value; };
const setShowW2ActionTip = value => { tipShown = value; };
const setNewDeviceAddress = value => { newAddress = value; };
const setShowAddDevice = value => { addShown = value; };
const syncDeviceList = () => { syncCalls++; };
const deleteDeviceByIndex = index => { deletedIndex = index; };
const openW2Editor = index => { editedIndex = index; };
const formatDeviceGroupText = value => value;
function text(node) { return node == null ? '' : typeof node === 'object' ? node.props.children.map(text).join('') : String(node); }
eval(compiled);
"""

def rules(css, name):
    body = re.search(re.escape('.' + name) + r' \{([^}]+)\}', css).group(1)
    return dict(re.findall(r'([a-z-]+):\s*([^;]+);', body))

def percent(value, parent):
    return float(value.rstrip('%')) * parent / 100

def main():
    blob = (ROOT / 'ui/main.ftu').read_bytes()
    ftu = json.loads(zlib.decompress(blob[struct.unpack_from('<I', blob, 6)[0] + 8:-6]))
    position = ftu['window__54']['position']
    report = []
    for panel in PANELS:
        css = (panel / 'src/pages/home/index.module.less').read_text(encoding='utf-8')
        source = (panel / 'src/pages/home/index.tsx').read_text(encoding='utf-8')
        page = rules(css, 'pagePanel') | rules(css, 'devicePage')
        actual = [percent(page[key], dim) for key, dim in [('left',1024),('top',600),('width',1024),('height',600)]]
        expected = [position[key] for key in ['left','top','width','height']]
        assert all(abs(a-b) < .001 for a,b in zip(actual, expected)), (actual, expected)
        header, body, row = [rules(css, name) for name in ['deviceHeaderRow','deviceList','deviceRow']]
        assert abs(percent(header['left'], 1007) - 19) < .001
        assert abs(percent(header['width'], 1007) - 980) < .001
        assert percent(header['top'], 400) == 20
        assert percent(header['height'], 400) == 52
        assert percent(body['top'], 400) == 72
        assert percent(body['height'], 400) == 297
        assert row['grid-template-columns'] == '145fr 190fr 155fr 190fr 140fr 160fr'
        assert row['height'] == 'min(5.80078125vw, 9.9vh)'
        assert row['font-size'] == 'min(2.34375vw, 4vh)'
        assert rules(css, 'deviceHeaderRow .deviceCell')['font-size'] == 'min(3.125vw, 5.333333vh)'
        assert rules(css, 'deviceCell')['height'] == '100%'
        assert rules(css, 'devicePageBackground')['pointer-events'] == 'none'
        assert (panel/'src/assets/cj96/w2_bgr_v2.png').read_bytes() == (ROOT/'resources/w2_bgr.png').read_bytes()
        encoded = re.search(r'base64,([^)]*)', (panel/'src/pages/home/device-font.less').read_text()).group(1)
        font = TTFont(io.BytesIO(base64.b64decode(encoded)))
        assert all(ord(c) in font.getBestCmap() for c in '地址名称类型阀组编号状态底部点击添加总线设备同步电磁关闭0123456789')
        result = subprocess.run([str(NODE), '-e', RUNNER, str(PANELS[0]), str(panel)], capture_output=True, text=True, encoding='utf-8')
        assert result.returncode == 0, result.stdout + result.stderr
        print(panel, result.stdout.strip())
        report.append({'panel': str(panel), 'native_geometry': 'pass', 'background': 'byte-identical', 'font': 'local font subset', 'handlers': result.stdout.strip(), 'phone_render': 'not tested'})
    out = ROOT/'diagnostics/app_device_list_20261006'
    out.mkdir(parents=True, exist_ok=True)
    (out/'verification.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print('PASS')

if __name__ == '__main__':
    main()
