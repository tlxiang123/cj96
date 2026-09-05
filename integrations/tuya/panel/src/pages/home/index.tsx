import React, { useCallback, useEffect, useRef, useState } from 'react';
import { Image, Input, ScrollView, Text, View, device, getStorageSync, hideStatusBar, setStorageSync, showToast } from '@ray-js/ray';
import { hooks, useDevInfo } from '@ray-js/panel-sdk';
import brandImage from '@/assets/cj96/set_button.png';
import networkNoneIcon from '@/assets/cj96/network_status_none_100.png';
import networkEthernetIcon from '@/assets/cj96/network_status_ethernet_100.png';
import networkWifiIcon from '@/assets/cj96/network_status_wifi_100.png';
import network4GIcon from '@/assets/cj96/network_status_4g_100.png';
import pressureIcon from '@/assets/cj96/window6_pressure_icon.png';
import flowIcon from '@/assets/cj96/window6_flow_icon.png';
import pumpIcon from '@/assets/cj96/window7_pump_icon.png';
import rainIcon from '@/assets/cj96/filter_icon_48.png';
import humidityIcon from '@/assets/cj96/water_icon_48.png';
import stopIcon from '@/assets/cj96/window8_stop_button_91x58.png';
import startIcon from '@/assets/cj96/window8_start_button_92x58.png';
import overviewIcon from '@/assets/cj96/bgr_btn1_ch.png';
import overviewNormalIcon from '@/assets/cj96/bgr_btn1.png';
import deviceIcon from '@/assets/cj96/bgr_btn2.png';
import deviceSelectedIcon from '@/assets/cj96/bgr_btn2_ch.png';
import planIcon from '@/assets/cj96/bgr_btn3.png';
import planSelectedIcon from '@/assets/cj96/bgr_btn3_ch.png';
import testIcon from '@/assets/cj96/bgr_btn4.png';
import testSelectedIcon from '@/assets/cj96/bgr_btn4_ch.png';
import logNavIcon from '@/assets/cj96/bgr_btn_log.png';
import logNavSelectedIcon from '@/assets/cj96/bgr_btn_log_ch.png';
import rainDialogBackground from '@/assets/cj96/rain_delay_dialog_520x250.png';
import humidityDialogBackground from '@/assets/cj96/humidity_threshold_dialog_520x250.png';
import rainDialogIcon from '@/assets/cj96/window3_delay_96.png';
import humidityDialogIcon from '@/assets/cj96/water_icon_96_aa.png';
import editBackground from '@/assets/cj96/debug_edit_inner_99x71.png';
import cancelButtonImage from '@/assets/cj96/set_runtime_cancel_same_font_120x60.png';
import okButtonImage from '@/assets/cj96/set_runtime_ok_same_font_120x60.png';
import sensorTypeIcon from '@/assets/cj96/Sensor.png';
import valveTypeIcon from '@/assets/cj96/Solenoid_Valve.png';
import toggleOffIcon from '@/assets/cj96/toggle-off.png';
import toggleOnIcon from '@/assets/cj96/toggle-on.png';
import window3CycleToggleOffIcon from '@/assets/cj96/window3_cycle_toggle_off_65.png';
import window3CycleToggleOnIcon from '@/assets/cj96/window3_cycle_toggle_on_65.png';
import window3EnableIcon from '@/assets/cj96/window3_enable_88.png';
import window3IntervalIcon from '@/assets/cj96/window3_interval_96.png';
import window3IntervalModeIcon from '@/assets/cj96/window3_interval_mode_100.png';
import window3ProgramIcon from '@/assets/cj96/window3_program_88.png';
import window3ProgramToggleOffIcon from '@/assets/cj96/window3_program_toggle_off_65.png';
import window3ProgramToggleOnIcon from '@/assets/cj96/window3_program_toggle_on_65.png';
import window3SprayProgramIcon from '@/assets/cj96/window3_spray_program_60.png';
import window3StartTimeIcon from '@/assets/cj96/window3_start_time_108.png';
import window3WeekModeIcon from '@/assets/cj96/window3_week_mode_108.png';
import window3Alarm1 from '@/assets/cj96/naozhong1.png';
import window3Alarm2 from '@/assets/cj96/naozhong2.png';
import window3Alarm3 from '@/assets/cj96/naozhong3.png';
import window3Alarm4 from '@/assets/cj96/naozhong4.png';
import window3Region1 from '@/assets/cj96/window3_region_1.png';
import window3Region2 from '@/assets/cj96/window3_region_2.png';
import window3Region3 from '@/assets/cj96/window3_region_3.png';
import window3Region4 from '@/assets/cj96/window3_region_4.png';
import window3Region5 from '@/assets/cj96/window3_region_5.png';
import window3Region6 from '@/assets/cj96/window3_region_6.png';
import window3Region7 from '@/assets/cj96/window3_region_7.png';
import cycleRegionLeft from '@/assets/cj96/cycle_region_left_390x340.png';
import cycleRegionRight from '@/assets/cj96/cycle_region_right_545x340.png';
import cycleColon from '@/assets/cj96/cycle_colon_14x40.png';
import cycleCancelImage from '@/assets/cj96/w2_group_bind_cancel_120x60.png';
import cycleConfirmImage from '@/assets/cj96/w2_ok_120x60.png';
import window4Region1 from '@/assets/cj96/window4_region1_319x384.png';
import window4Region2 from '@/assets/cj96/window4_region2_319x384.png';
import window4Region3 from '@/assets/cj96/window4_region3_321x384.png';
import addDeviceDialogBackground from '@/assets/cj96/w2_add_device_dialog_650x300.png';
import deviceListBackground from '@/assets/cj96/w2_bgr_v2.png';
import tipDialogBackground from '@/assets/cj96/w2_group_rename_dialog_560x220.png';
import actionTipBackground from '@/assets/cj96/w2_tip_center_560x140.png';
import sensorTypeSelectedIcon from '@/assets/cj96/Sensor_Selected.png';
import valveTypeSelectedIcon from '@/assets/cj96/Solenoid_Valve_Selected.png';
import w2SetRegion1 from '@/assets/cj96/w2set_region_1.png';
import w2SetRegion2 from '@/assets/cj96/w2set_region_2.png';
import w2SetRegion3 from '@/assets/cj96/w2set_region_3.png';
import w2ChangeIrrListBackground from '@/assets/cj96/w2_change_irr_list_546x184.png';
import w2OneClickAddImage from '@/assets/cj96/w2set_one_click_add_color_match_120x60.png';
import w2GroupBindCancelImage from '@/assets/cj96/w2_group_bind_cancel_120x60.png';
import w2SetOkImage from '@/assets/cj96/w2set_ok_color_match_120x60.png';
import w2BindGroupImage from '@/assets/cj96/w2_set_bind_113x113_norm.png';
import w2ClearGroupImage from '@/assets/cj96/w2_set_clear_113x113_norm.png';
import w2DeleteGroupImage from '@/assets/cj96/w2_set_delete_group_113x113_norm.png';
import w2RenameGroupImage from '@/assets/cj96/w2_set_rename_group_113x113_norm.png';
import w2IrrLabelImage from '@/assets/cj96/w2_set_irr_label_borderless_113x69.png';
import w2AddressCombinedImage from '@/assets/cj96/w2_set_address_combined_001.png';
import w2DeviceNameLabelImage from '@/assets/cj96/w2_set_device_name_label_77x77.png';
import w2GroupBindRegion1 from '@/assets/cj96/w2_group_bind_region1_405x305.png';
import w2GroupBindRegion2 from '@/assets/cj96/w2_group_bind_region2_245x305.png';
import w2GroupBindRegion3 from '@/assets/cj96/w2_group_bind_region3_309x305.png';
import w2GroupBindAction from '@/assets/cj96/w2_group_bind_action_983x70.png';
import w2BindGroupNumber from '@/assets/cj96/w2_bind_group_number_145x70.png';
import w2BindGroupName from '@/assets/cj96/w2_bind_group_name_145x70.png';
import w2BindLinkedPump from '@/assets/cj96/w2_bind_linked_pump_145x70.png';
import w2BindLinkedSensor from '@/assets/cj96/w2_bind_linked_sensor_145x70.png';
import w2BindSelectPump from '@/assets/cj96/w2_bind_select_pump_215x105.png';
import w2BindSelectSensor from '@/assets/cj96/w2_bind_select_sensor_215x105.png';
import w2GroupBindConfirm from '@/assets/cj96/w2_group_bind_confirm_120x60.png';
import w2ClearAllImage from '@/assets/cj96/w2_clear_all_120x60.png';
import w2GroupRenameEdit from '@/assets/cj96/w2_group_rename_edit_360x61.png';
import w2CapacityCancelImage from '@/assets/cj96/w2_group_bind_cancel_120x60.png';
import w2CapacityOkImage from '@/assets/cj96/w2_ok_120x60.png';
import selectOffImage from '@/assets/cj96/window7_select_off.png';
import selectOnImage from '@/assets/cj96/window7_select_on.png';
import modeSelectOffImage from '@/assets/cj96/mode_select_off.png';
import modeSelectOnImage from '@/assets/cj96/mode_select_on.png';
import styles from './index.module.less';

const CJ96_DP_ID = 101;
const PANEL_VERSION = '1.0.55';
const ROUND_IRRIGATION_ON_FRAME = 'AA55F10155AA';
const ROUND_IRRIGATION_OFF_FRAME = 'AA55F10255AA';
const WINDOW4_DEVICE_STATE_COMMAND = 'E4';
const WINDOW4_GROUP_STATE_COMMAND = 'E5';
const HOME_STOP_SCHEDULE_FRAME = 'AA55F20155AA';
const HOME_ADVANCE_GROUP_FRAME = 'AA55F20255AA';
const DEVICE_SYNC_FRAME = 'AA55F50155AA';
const DEVICE_TABLE_READ_FRAME = 'AA55F50255AA';
const DEVICE_TABLE_VERSION_PREFIX = 'AA55F503';
// The board scans addresses 20..255 in a fixed 14.86-second window.
// Keep the longer watchdog internal so table frames can finish after scanning,
// while the user-facing modal shows the actual fixed scan maximum.
const DEVICE_SYNC_TIMEOUT_SECONDS = 90;
const DEVICE_DISCOVERY_DISPLAY_SECONDS = 15;
const DEVICE_TABLE_VERSION_FALLBACK_MS = 1800;
const NETWORK_NONE_STATE_FRAME = 'AA55E10055AA';
const NETWORK_ETHERNET_STATE_FRAME = 'AA55E10155AA';
const NETWORK_WIFI_STATE_FRAME = 'AA55E10255AA';
const NETWORK_4G_STATE_FRAME = 'AA55E10355AA';
const WEEKDAYS = ['日', '一', '二', '三', '四', '五', '六'];
const WEEKDAY_OPTIONS = ['日', '一', '二', '三', '四', '五', '六'];
const WINDOW3_ALARM_ICONS = [window3Alarm1, window3Alarm2, window3Alarm3, window3Alarm4];

type MainPage = 'overview' | 'device' | 'plan' | 'test' | 'log';

type W2CapacityMode = 'single' | 'all';
type W2GroupChoiceMode = 'bind' | 'clear' | 'delete' | 'rename';

type W2CapacityDialogSnapshot = {
  mode: W2CapacityMode;
  groupNo: number;
  pending: number;
  last: number;
  capacities: Record<number, number>;
};

type CycleRange = {
  startHour: string;
  startMinute: string;
  endHour: string;
  endMinute: string;
};

type CycleDuration = {
  hour: string;
  minute: string;
  second: string;
};

type PanelDevice = {
  address: number;
  type: '水泵' | '电磁阀' | '传感器';
  name: string;
  group: string;
  connected: boolean;
  stateKnown: boolean;
  state: boolean;
  status: string;
};

const DEVICE_TABLE_CACHE_SCHEMA = 4;
const DEVICE_TABLE_CACHE_PREFIX = 'cj96:device-table:';
const VALVE_LOG_CACHE_SCHEMA = 2;
const VALVE_LOG_CACHE_PREFIX = 'cj96:valve-log:';
const MAX_VALVE_LOG_ENTRIES = 100;
const VALVE_LOG_RETENTION_MS = 30 * 24 * 60 * 60 * 1000;

type ValveLogMode = '手动灌溉' | '自动灌溉';

type ValveLogEntry = {
  id: string;
  timestamp: number;
  time: string;
  week: string;
  mode: ValveLogMode;
  action: '开启' | '关闭';
  detail: string;
};

function valveLogCacheKey(deviceId: string): string {
  return VALVE_LOG_CACHE_PREFIX + deviceId;
}

function isValveLogEntry(value: any): value is ValveLogEntry {
  return Boolean(value) && typeof value.id === 'string' &&
    Number.isFinite(value.timestamp) && typeof value.time === 'string' &&
    typeof value.week === 'string' &&
    (value.mode === '手动灌溉' || value.mode === '自动灌溉') &&
    (value.action === '开启' || value.action === '关闭') &&
    typeof value.detail === 'string';
}

function pruneValveLogs(entries: ValveLogEntry[], now = Date.now()): ValveLogEntry[] {
  return entries
    .filter(entry => now - entry.timestamp < VALVE_LOG_RETENTION_MS)
    .sort((left, right) => right.timestamp - left.timestamp)
    .slice(0, MAX_VALVE_LOG_ENTRIES);
}

function readValveLogCache(deviceId: string): ValveLogEntry[] {
  if (!deviceId) return [];
  try {
    const result: any = getStorageSync({ key: valveLogCacheKey(deviceId) });
    const raw = typeof result === 'string' ? result : result?.data;
    const parsed = typeof raw === 'string' ? JSON.parse(raw) : raw;
    if (!parsed || parsed.schema !== VALVE_LOG_CACHE_SCHEMA || !Array.isArray(parsed.entries)) return [];
    return pruneValveLogs(parsed.entries.filter(isValveLogEntry));
  } catch (_error) {
    return [];
  }
}

function writeValveLogCache(deviceId: string, entries: ValveLogEntry[]): void {
  if (!deviceId) return;
  try {
    setStorageSync({
      key: valveLogCacheKey(deviceId),
      data: JSON.stringify({ schema: VALVE_LOG_CACHE_SCHEMA, entries: pruneValveLogs(entries) }),
    });
  } catch (_error) {
    // A failed local cache must not prevent the command from reaching the board.
  }
}

type DeviceTableCache = {
  schema: number;
  deviceId: string;
  hash: number;
  devices: PanelDevice[];
  groupCapacities: Record<number, number>;
  savedAt: number;
};

function deviceTableCacheKey(deviceId: string): string {
  return DEVICE_TABLE_CACHE_PREFIX + deviceId;
}

function isPanelDevice(value: any): value is PanelDevice {
  return Boolean(value) &&
    Number.isInteger(value.address) && value.address >= 1 && value.address <= 255 &&
    (value.type === '水泵' || value.type === '电磁阀' || value.type === '传感器') &&
    typeof value.name === 'string' && typeof value.group === 'string' &&
    typeof value.connected === 'boolean' && typeof value.stateKnown === 'boolean' &&
    typeof value.state === 'boolean' && typeof value.status === 'string';
}

function isGroupCapacities(value: any): value is Record<number, number> {
  if (!value || typeof value !== 'object' || Array.isArray(value)) return false;
  for (let groupNo = 1; groupNo <= 128; groupNo += 1) {
    const capacity = value[groupNo];
    if (!Number.isInteger(capacity) || capacity < 1 || capacity > 3) return false;
  }
  return true;
}

function groupCapacityBytes(capacities: Record<number, number>): number[] {
  return Array.from({ length: 128 }, (_, index) => clamp(Number(capacities[index + 1] ?? 2), 1, 3));
}

function combinedDeviceTableHash(tableBytes: number[], capacities: Record<number, number>): number {
  return fnv1a(tableBytes.concat(groupCapacityBytes(capacities)));
}

function decodeGroupCapacities(bytes: number[]): Record<number, number> | null {
  if (bytes.length !== 128 || bytes.some(value => value < 1 || value > 3)) return null;
  const result: Record<number, number> = {};
  bytes.forEach((value, index) => { result[index + 1] = value; });
  return result;
}

function readDeviceTableCache(deviceId: string): DeviceTableCache | null {
  if (!deviceId) return null;
  try {
    const result: any = getStorageSync({ key: deviceTableCacheKey(deviceId) });
    const raw = typeof result === 'string' ? result : result?.data;
    const parsed = typeof raw === 'string' ? JSON.parse(raw) : raw;
    if (!parsed || parsed.schema !== DEVICE_TABLE_CACHE_SCHEMA ||
        parsed.deviceId !== deviceId || !Number.isInteger(parsed.hash) ||
        parsed.hash < 0 || parsed.hash > 0xFFFFFFFF || !Array.isArray(parsed.devices) ||
        !parsed.devices.every(isPanelDevice) || !isGroupCapacities(parsed.groupCapacities)) {
      return null;
    }
    return {
      schema: parsed.schema,
      deviceId: parsed.deviceId,
      hash: parsed.hash >>> 0,
      devices: sortPanelDevices(parsed.devices),
      groupCapacities: parsed.groupCapacities,
      savedAt: Number.isFinite(parsed.savedAt) ? parsed.savedAt : 0,
    };
  } catch (_error) {
    return null;
  }
}

function writeDeviceTableCache(deviceId: string, hash: number, devices: PanelDevice[],
  groupCapacities: Record<number, number>): DeviceTableCache | null {
  if (!deviceId || !Number.isInteger(hash) || hash < 0 || hash > 0xFFFFFFFF ||
      !Array.isArray(devices) || !devices.every(isPanelDevice) ||
      !isGroupCapacities(groupCapacities)) return null;
  const cache: DeviceTableCache = {
    schema: DEVICE_TABLE_CACHE_SCHEMA,
    deviceId,
    hash: hash >>> 0,
    devices: sortPanelDevices(devices),
    groupCapacities: { ...groupCapacities },
    savedAt: Date.now(),
  };
  try {
    setStorageSync({
      key: deviceTableCacheKey(deviceId),
      data: JSON.stringify(cache),
    });
    return cache;
  } catch (_error) {
    return null;
  }
}

const INITIAL_DEVICES: PanelDevice[] = [
  { address: 1, type: '水泵', name: '水泵1', group: '-', connected: false, stateKnown: false, state: false, status: '未连接' },
  { address: 2, type: '水泵', name: '水泵2', group: '-', connected: false, stateKnown: false, state: false, status: '未连接' },
  { address: 6, type: '传感器', name: '湿度传感器', group: '-', connected: false, stateKnown: false, state: false, status: '未连接' },
  { address: 8, type: '传感器', name: '雨量传感器', group: '-', connected: false, stateKnown: false, state: false, status: '未连接' },
  { address: 9, type: '传感器', name: '水压表', group: '-', connected: false, stateKnown: false, state: false, status: '未连接' },
  { address: 10, type: '传感器', name: '流量表', group: '-', connected: false, stateKnown: false, state: false, status: '未连接' },
];

type DeviceListRow =
  | { kind: 'device'; device: PanelDevice; index: number }
  | { kind: 'empty'; index: number };

const DEFAULT_DEVICE_COUNT = 6;
const DEVICE_LIST_HEADERS = ['地址', '名称', '类型', '阀组编号', '状态', '操作'];
const DEVICE_SYNC_PREFIX = 'CJDS1';
const COMMAND_ACK_PREFIX = 'CJACK1';
const BOARD_COMMAND_ACK_TIMEOUT_MS = 10000;
const COMMAND_ACK_STORAGE_PREFIX = 'cj96.commandAckSeq.';
const MAX_SYNC_DEVICE_COUNT = 242;

type PendingBoardCommand = {
  kind: string;
  command: string;
  actionName: string;
  showSuccessToast: boolean;
  minimumSequence: number;
  startedAt: number;
  expectsDeviceTable?: boolean;
  usesDeviceTableRevision?: boolean;
  onSuccess?: () => void;
  onFailure?: () => void;
};

type DeviceSyncTransfer = {
  session: string;
  count: number;
  totalChunks: number;
  dataHash: number;
  groupCapacities: Record<number, number>;
  chunks: Record<number, string>;
  completed: boolean;
  decoded?: PanelDevice[];
};

function fnv1a(bytes: number[]): number {
  let hash = 0x811c9dc5;
  bytes.forEach(byte => {
    hash ^= byte & 0xff;
    hash = Math.imul(hash, 0x01000193) >>> 0;
  });
  return hash >>> 0;
}

function asciiBytes(value: string): number[] {
  return Array.from(value).map(character => character.charCodeAt(0) & 0xff);
}

function hexToBytes(value: string): number[] | null {
  if (value.length % 2 !== 0 || !/^[0-9A-F]*$/.test(value)) return null;
  const bytes: number[] = [];
  for (let index = 0; index < value.length; index += 2) {
    bytes.push(parseInt(value.slice(index, index + 2), 16));
  }
  return bytes;
}

function decodeUtf8(bytes: number[]): string {
  if (!bytes.length) return '';
  try {
    return decodeURIComponent(bytes.map(byte => `%${byte.toString(16).padStart(2, '0')}`).join(''));
  } catch (_error) {
    return bytes.map(byte => String.fromCharCode(byte)).join('');
  }
}

function decodeGroups(mode: number, bitmap: number[]): string {
  if (mode === 2) return '*';
  if (mode !== 1) return '-';
  const groups: number[] = [];
  bitmap.forEach((byte, byteIndex) => {
    for (let bit = 0; bit < 8; bit += 1) {
      if ((byte & (1 << bit)) !== 0) groups.push(byteIndex * 8 + bit + 1);
    }
  });
  return groups.length ? groups.join(',') : '-';
}

function sortPanelDevices(items: PanelDevice[]): PanelDevice[] {
  return items
    .map((item, originalIndex) => ({ item, originalIndex }))
    .sort((left, right) => left.item.address - right.item.address || left.originalIndex - right.originalIndex)
    .map(entry => entry.item);
}

function decodeDeviceTable(bytes: number[], expectedCount: number): PanelDevice[] | null {
  const devices: PanelDevice[] = [];
  let offset = 0;
  while (offset < bytes.length) {
    if (offset + 21 > bytes.length) return null;
    const address = bytes[offset++];
    const typeCode = bytes[offset++];
    const flags = bytes[offset++];
    const groupMode = bytes[offset++];
    const groupBitmap = bytes.slice(offset, offset + 16);
    offset += 16;
    const nameLength = bytes[offset++];
    if (offset + nameLength + 1 > bytes.length) return null;
    const name = decodeUtf8(bytes.slice(offset, offset + nameLength));
    offset += nameLength;
    const statusLength = bytes[offset++];
    if (offset + statusLength > bytes.length) return null;
    const status = decodeUtf8(bytes.slice(offset, offset + statusLength));
    offset += statusLength;
    const type: PanelDevice['type'] = typeCode === 1 ? '水泵' : typeCode === 2 ? '电磁阀' : '传感器';
    devices.push({
      address,
      type,
      name,
      group: decodeGroups(groupMode, groupBitmap),
      connected: (flags & 1) !== 0,
      stateKnown: (flags & 2) !== 0,
      state: (flags & 4) !== 0,
      status,
    });
  }
  return devices.length === expectedCount ? sortPanelDevices(devices) : null;
}

function pad2(value: number): string {
  return String(value).padStart(2, '0');
}

function formatCallbackValue(value: any): string {
  return String(value?.errorMsg || value?.message || value?.errorCode || value?.code || value);
}

function clamp(value: number, minimum: number, maximum: number): number {
  return Math.min(maximum, Math.max(minimum, value));
}

function extractGroupNumbers(group: string): number[] {
  return Array.from(new Set(
    Array.from(String(group || '').matchAll(/\d+/g))
      .map(match => parseInt(match[0], 10))
      .filter(groupNo => groupNo >= 1 && groupNo <= 128)
  )).sort((a, b) => a - b);
}

function formatValveLogDetail(items: PanelDevice[]): string {
  const grouped = new Map<string, number[]>();
  items.filter(item => item.type === '电磁阀').forEach(item => {
    const normalized = String(item.group || '').trim();
    const groups = normalized === '*' || normalized === '所有阀组'
      ? ['全部阀组']
      : extractGroupNumbers(normalized).map(groupNo => `阀组${groupNo}`);
    (groups.length ? groups : ['未分组']).forEach(group => {
      const addresses = grouped.get(group) || [];
      addresses.push(item.address);
      grouped.set(group, addresses);
    });
  });
  if (!grouped.size) return '暂无已配置电磁阀';
  return Array.from(grouped.entries())
    .sort(([left], [right]) => left.localeCompare(right, 'zh-CN'))
    .map(([group, addresses]) => {
      const uniqueAddresses = Array.from(new Set(addresses)).sort((left, right) => left - right);
      return `${group} [地址${uniqueAddresses.join('、')}]`;
    })
    .join('；');
}

function formatDeviceGroupText(group: string): string {
  const normalized = String(group || '').trim();
  if (!normalized || normalized === '-') return '-';
  if (normalized === '*' || normalized === '所有阀组') return '*';
  const numbers = extractGroupNumbers(normalized);
  return numbers.length ? numbers.join(',') : '-';
}

function collectDeviceGroupNumbers(items: PanelDevice[]): number[] {
  return Array.from(new Set(items.flatMap(item => extractGroupNumbers(item.group))))
    .sort((a, b) => a - b);
}

function normalizeNormalSyncDevices(items: PanelDevice[], baselineAddresses: Set<number> | null,
  autoAssignPending: boolean): PanelDevice[] {
  if (autoAssignPending || !baselineAddresses) return items;

  // A normal sync only discovers/updates devices. A newly discovered custom
  // device must start outside every valve group; only the explicit bind or
  // one-click-add action may assign it to a group.
  return items.map(item => item.address >= 20 && !baselineAddresses.has(item.address)
    ? { ...item, group: '-' }
    : item);
}



function formatPanelGroupSummary(groupNo: number, items: PanelDevice[], editingIndex: number | null, previewEnabled: boolean, editingTypeIndex: number, editingAddress: number): string {
  const members = items.filter((item, index) => {
    // Keep a persisted device in the group summary while its editor is open.
    // Hide only the old record when its address/type is being replaced by the
    // editor preview below.
    if (index === editingIndex && editingIndex !== null && editingIndex >= DEFAULT_DEVICE_COUNT) {
      const editingType: PanelDevice['type'] = editingTypeIndex === 1 ? '传感器' : '电磁阀';
      const identityChanged = item.address !== editingAddress || item.type !== editingType;
      if (identityChanged) return false;
    }
    const normalized = String(item.group || '').trim();
    return normalized === '*' || normalized === '\u6240\u6709\u9600\u7ec4' || extractGroupNumbers(normalized).includes(groupNo);
  });
  const valveAddresses = members.filter(item => item.type === '\u7535\u78c1\u9600').map(item => item.address).sort((a, b) => a - b);
  const pumpAddresses = members.filter(item => item.type === '\u6c34\u6cf5').map(item => item.address).sort((a, b) => a - b);
  const sensorNames = members.filter(item => item.type === '传感器').map(item => {
    const fallback = '传感器[' + item.address + ']';
    const name = String(item.name || '').trim();
    if (!name || name === '传感器') return fallback;
    const shortName = name.endsWith('传感器') ? name.slice(0, -3).trim() : name;
    return shortName || fallback;
  });
  if (previewEnabled && editingTypeIndex === 0 && groupNo > 0) {
    valveAddresses.push(editingAddress);
    valveAddresses.sort((a, b) => a - b);
  }
  const parts: string[] = [];
  if (valveAddresses.length) parts.push(`\u7535\u78c1\u9600[${Array.from(new Set(valveAddresses)).join(', ')}]`);
  const pumpSensorParts: string[] = [];
  if (pumpAddresses.length) pumpSensorParts.push(`\u6c34\u6cf5${Array.from(new Set(pumpAddresses)).join('\uff0c')}`);
  if (sensorNames.length) pumpSensorParts.push(Array.from(new Set(sensorNames)).join(' '));
  if (pumpSensorParts.length) parts.push(`[${pumpSensorParts.join(' ')}]`);
  return parts.length ? parts.join(' ') : '[\u7a7a]';
}

function bindPanelDeviceGroup(item: PanelDevice, groupNo: number, allGroups: boolean): string {
  if (allGroups) return '*';
  if (item.type === '电磁阀') return String(groupNo);
  const normalized = String(item.group || '').trim();
  if (normalized === '*') return '*';
  const groups = extractGroupNumbers(normalized);
  if (!groups.includes(groupNo)) groups.push(groupNo);
  return groups.sort((a, b) => a - b).join(',') || String(groupNo);
}

function removeGroupNumber(group: string, targetGroupNo: number): string {
  const normalized = String(group || '').trim();
  if (!normalized || normalized === '-' || normalized === '*' || normalized === '所有阀组') {
    return normalized || '-';
  }
  const remaining = extractGroupNumbers(normalized).filter(groupNo => groupNo !== targetGroupNo);
  return remaining.length ? remaining.join(',') : '-';
}

function makeByteCommandFrame(command: 'F3' | 'F4' | 'F6' | 'F7', value: number): string {
  return `AA55${command}${value.toString(16).padStart(2, '0').toUpperCase()}55AA`;
}

function makeWindow4StateFrame(command: string, value: number, enabled: boolean): string {
  return `AA55${command}${value.toString(16).padStart(2, '0').toUpperCase()}${enabled ? '01' : '00'}55AA`;
}

function makeW2CapacityCommand(mode: W2CapacityMode, groupNo: number, capacity: number): string {
  const capacityHex = capacity.toString(16).padStart(2, '0').toUpperCase();
  if (mode === 'all') return `AA55F9${capacityHex}55AA`;
  const groupHex = groupNo.toString(16).padStart(2, '0').toUpperCase();
  return `AA55F8${groupHex}${capacityHex}55AA`;
}

// Bind the selected addresses on the board in one frame.  The previous APP
// implementation only changed React state, so the board kept its old arre
// value and the next table report restored the old group numbers.
function makeW2GroupBindCommand(addresses: number[], groupNo: number, allGroups: boolean): string {
  const validAddresses = Array.from(new Set(addresses))
    .map(address => Math.trunc(address))
    .filter(address => address >= 1 && address <= 255)
    .slice(0, 60);
  const groupCode = allGroups ? 0xFF : clamp(Math.trunc(groupNo), 1, 128);
  const addressHex = validAddresses
    .map(address => address.toString(16).padStart(2, '0').toUpperCase())
    .join('');
  return `AA55FA${groupCode.toString(16).padStart(2, '0').toUpperCase()}${validAddresses.length.toString(16).padStart(2, '0').toUpperCase()}${addressHex}55AA`;
}

function makeW2GroupScopeCommand(command: 'FB' | 'FC', groupNo: number, allGroups = false): string {
  const groupCode = allGroups ? 0xFF : clamp(Math.trunc(groupNo), 1, 128);
  return `AA55${command}${groupCode.toString(16).padStart(2, '0').toUpperCase()}55AA`;
}

function encodeUtf8Hex(value: string): string {
  const bytes: number[] = [];
  for (const character of Array.from(value)) {
    const codePoint = character.codePointAt(0) || 0;
    if (codePoint <= 0x7f) {
      bytes.push(codePoint);
    } else if (codePoint <= 0x7ff) {
      bytes.push(0xc0 | (codePoint >> 6), 0x80 | (codePoint & 0x3f));
    } else if (codePoint <= 0xffff) {
      bytes.push(0xe0 | (codePoint >> 12), 0x80 | ((codePoint >> 6) & 0x3f), 0x80 | (codePoint & 0x3f));
    } else {
      bytes.push(0xf0 | (codePoint >> 18), 0x80 | ((codePoint >> 12) & 0x3f), 0x80 | ((codePoint >> 6) & 0x3f), 0x80 | (codePoint & 0x3f));
    }
  }
  return bytes.map(byte => byte.toString(16).padStart(2, '0').toUpperCase()).join('');
}

function makeW2GroupRenameCommand(groupNo: number, name: string): string {
  const groupHex = clamp(Math.trunc(groupNo), 1, 128).toString(16).padStart(2, '0').toUpperCase();
  const nameHex = encodeUtf8Hex(name).slice(0, 64);
  return `AA55FD${groupHex}${(nameHex.length / 2).toString(16).padStart(2, '0').toUpperCase()}${nameHex}55AA`;
}

function truncateUtf8Text(value: string, maximumBytes: number): string {
  let result = '';
  let usedBytes = 0;
  for (const character of Array.from(value)) {
    const bytes = encodeUtf8Hex(character).length / 2;
    if (usedBytes + bytes > maximumBytes) break;
    result += character;
    usedBytes += bytes;
  }
  return result;
}

function makeW2DeviceUpsertCommand(
  address: number,
  type: PanelDevice['type'],
  group: string,
  name: string,
): string {
  const normalizedAddress = clamp(Math.trunc(address), 20, 255);
  const typeCode = type === '传感器' ? 3 : 2;
  const normalizedGroup = String(group || '-').trim() || '-';
  const groupHex = encodeUtf8Hex(normalizedGroup);
  const nameHex = encodeUtf8Hex(truncateUtf8Text(name, 19));
  return `AA55FE${normalizedAddress.toString(16).padStart(2, '0').toUpperCase()}${typeCode.toString(16).padStart(2, '0').toUpperCase()}${(groupHex.length / 2).toString(16).padStart(2, '0').toUpperCase()}${groupHex}${(nameHex.length / 2).toString(16).padStart(2, '0').toUpperCase()}${nameHex}55AA`;
}

function makeDeviceTableVersionFrame(hash: number): string {
  return `${DEVICE_TABLE_VERSION_PREFIX}${(hash >>> 0).toString(16).padStart(8, '0').toUpperCase()}55AA`;
}

function appendDeviceTableBaseHash(frame: string, hash: number): string {
  const suffix = '55AA';
  if (!frame.endsWith(suffix)) return frame;
  return `${frame.slice(0, -suffix.length)}EE${(hash >>> 0).toString(16).padStart(8, '0').toUpperCase()}${suffix}`;
}

export function Home() {
  const { publishDpsBase } = device;
  const devInfo = useDevInfo();
  const deviceOnline = hooks.useDeviceOnline();
  const reportedFrame = hooks.useDpValue<string>('cj96_raw');
  const [now, setNow] = useState(() => new Date());
  const [sending, setSending] = useState(false);
  const sendingRef = useRef(false);
  const [irrigating, setIrrigating] = useState(false);
  const [reportedNetwork, setReportedNetwork] = useState<'none' | 'ethernet' | 'wifi' | '4g'>('none');
  const [activeDialog, setActiveDialog] = useState<'rain' | 'humidity' | null>(null);
  const [rainDelayDays, setRainDelayDays] = useState(1);
  const [humidityThreshold, setHumidityThreshold] = useState(80);
  const [rainInput, setRainInput] = useState('1');
  const [humidityInput, setHumidityInput] = useState('80');
  const [activePage, setActivePage] = useState<MainPage>('overview');
  const [devices, setDevices] = useState<PanelDevice[]>(INITIAL_DEVICES);
  const [valveLogs, setValveLogs] = useState<ValveLogEntry[]>([]);
  const [showAddDevice, setShowAddDevice] = useState(false);
  const [syncingDevices, setSyncingDevices] = useState(false);
  const [syncRemaining, setSyncRemaining] = useState(0);
  const [deviceReportPending, setDeviceReportPending] = useState(false);
  const [window2Opening, setWindow2Opening] = useState(false);
  const [window2TransitionClosing, setWindow2TransitionClosing] = useState(false);
  const syncTransferRef = useRef<DeviceSyncTransfer | null>(null);
  const syncDeadlineRef = useRef(0);
  const syncDisplayDeadlineRef = useRef(0);
  const syncRequestActiveRef = useRef(false);
  const syncButtonLockRef = useRef(false);
  const pendingDeleteAddressRef = useRef<number | null>(null);
  const pendingAutoAssignRef = useRef(false);
  const deviceTableReadPendingRef = useRef(false);
  const deviceTableVersionFallbackTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const window2OpeningRef = useRef(false);
  const window2TransitionTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const window2TransitionCloseTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);
  const deviceTableHashRef = useRef<number | null>(null);
  const deviceTableCacheRef = useRef<DeviceTableCache | null>(null);
  const syncGroupCapacitiesRef = useRef<Record<number, number> | null>(null);
  const cacheLoadedDeviceIdRef = useRef<string | null>(null);
  const syncBaselineAddressesRef = useRef<Set<number> | null>(null);
  const pendingBoardCommandRef = useRef<PendingBoardCommand | null>(null);
  const lastAckSequenceRef = useRef(0);
  const [showW2Editor, setShowW2Editor] = useState(false);
  const [showW2GroupBind, setShowW2GroupBind] = useState(false);
  const [showW2GroupBindScope, setShowW2GroupBindScope] = useState(false);
  const [showW2GroupChoice, setShowW2GroupChoice] = useState(false);
  const [w2GroupChoiceMode, setW2GroupChoiceMode] = useState<W2GroupChoiceMode>('bind');
  const [w2GroupChoiceNo, setW2GroupChoiceNo] = useState(1);
  const [showW2Capacity, setShowW2Capacity] = useState(false);
  const [showW2Rename, setShowW2Rename] = useState(false);
  const [showW2DeviceRename, setShowW2DeviceRename] = useState(false);
  const [showW2ClearTip, setShowW2ClearTip] = useState(false);
  const [showW2ActionTip, setShowW2ActionTip] = useState(false);
  const [w2TipText, setW2TipText] = useState('');
  const [w2EditingIndex, setW2EditingIndex] = useState<number | null>(null);
  const [w2EditingAddress, setW2EditingAddress] = useState('20');
  const [w2EditingName, setW2EditingName] = useState('');
  const [w2DeviceNameDraft, setW2DeviceNameDraft] = useState('');
  const [w2EditingTypeIndex, setW2EditingTypeIndex] = useState(0);
  const [w2SelectedGroupNo, setW2SelectedGroupNo] = useState(1);
  const [w2GroupPreviewEnabled, setW2GroupPreviewEnabled] = useState(false);
  const [w2GroupNumbers, setW2GroupNumbers] = useState([1, 2, 3, 4]);
  const [w2GroupName, setW2GroupName] = useState('阀组[1]');
  const [w2CapacityMode, setW2CapacityMode] = useState<W2CapacityMode>('single');
  const [w2CapacityGroupNo, setW2CapacityGroupNo] = useState(1);
  const [w2CapacityPending, setW2CapacityPending] = useState(2);
  const [w2CapacityLast, setW2CapacityLast] = useState(2);
  const [w2GroupCapacities, setW2GroupCapacities] = useState<Record<number, number>>({});
  const [w2SelectedPumpIndexes, setW2SelectedPumpIndexes] = useState<number[]>([]);
  const [w2SelectedSensorIndexes, setW2SelectedSensorIndexes] = useState<number[]>([]);
  const [w2GroupBindAllGroups, setW2GroupBindAllGroups] = useState(false);
  const w2AutoAssignSnapshotRef = useRef<PanelDevice[] | null>(null);
  const w2CapacityDialogSnapshotRef = useRef<W2CapacityDialogSnapshot | null>(null);
  const [newDeviceAddress, setNewDeviceAddress] = useState('20');
  const [newDeviceType, setNewDeviceType] = useState<'传感器' | '电磁阀'>('传感器');
  const [programIndex, setProgramIndex] = useState(1);
  const [programEnabled, setProgramEnabled] = useState(false);
  const [cycleEnabled, setCycleEnabled] = useState(false);
  const [showCycleWindow, setShowCycleWindow] = useState(false);
  const [cycleRanges, setCycleRanges] = useState<CycleRange[]>([
    { startHour: '', startMinute: '', endHour: '', endMinute: '' },
    { startHour: '', startMinute: '', endHour: '', endMinute: '' },
  ]);
  const [cycleIrrigation, setCycleIrrigation] = useState<CycleDuration>({ hour: '0', minute: '0', second: '0' });
  const [cycleSoak, setCycleSoak] = useState<CycleDuration>({ hour: '0', minute: '0', second: '0' });
  const [cycleIntervalDays, setCycleIntervalDays] = useState('0');
  const [cycleCount, setCycleCount] = useState('');
  const [cycleCountInitialized, setCycleCountInitialized] = useState(false);
  const [activeCycleInput, setActiveCycleInput] = useState<string | null>(null);
  const [startTimes, setStartTimes] = useState(['--:--', '--:--', '--:--', '--:--']);
  const [weekMode, setWeekMode] = useState(true);
  const [selectedWeekdays, setSelectedWeekdays] = useState<number[]>([]);
  const [intervalDays, setIntervalDays] = useState('-');
  const [groupDurations, setGroupDurations] = useState(['', '', '', '']);
  const [groupTestStates, setGroupTestStates] = useState<boolean[]>(Array(4).fill(false));
  const [roundMinutes, setRoundMinutes] = useState('0');
  const [roundSeconds, setRoundSeconds] = useState('0');
  const [showRoundConfirm, setShowRoundConfirm] = useState(false);
  const normalizedReportedFrame = String(reportedFrame || '').toUpperCase();
  const deviceId = devInfo?.devId;
  const completeWindow2Opening = useCallback(() => {
    if (window2TransitionTimerRef.current !== null) {
      clearTimeout(window2TransitionTimerRef.current);
      window2TransitionTimerRef.current = null;
    }
    if (!window2OpeningRef.current) return;
    window2OpeningRef.current = false;
    setWindow2TransitionClosing(true);
    if (window2TransitionCloseTimerRef.current !== null) {
      clearTimeout(window2TransitionCloseTimerRef.current);
    }
    window2TransitionCloseTimerRef.current = setTimeout(() => {
      window2TransitionCloseTimerRef.current = null;
      setWindow2Opening(false);
      setWindow2TransitionClosing(false);
    }, 190);
  }, []);

  const publishSyncAck = (session: string, token: number) => {
    if (!deviceId) return;
    publishDpsBase({
      deviceId,
      dps: { [CJ96_DP_ID]: `${DEVICE_SYNC_PREFIX}|A|${session}|${token}` },
      mode: 1,
      pipelines: [0, 1, 2, 3, 4, 5, 6],
      options: {},
      success: () => {},
      fail: () => {},
    });
  };

  useEffect(() => {
    hideStatusBar();
    const timer = setInterval(() => setNow(new Date()), 1000);
    return () => {
      clearInterval(timer);
      if (deviceTableVersionFallbackTimerRef.current !== null) {
        clearTimeout(deviceTableVersionFallbackTimerRef.current);
        deviceTableVersionFallbackTimerRef.current = null;
      }
    };
  }, []);

  useEffect(() => () => {
    if (window2TransitionTimerRef.current !== null) {
      clearTimeout(window2TransitionTimerRef.current);
    }
    if (window2TransitionCloseTimerRef.current !== null) {
      clearTimeout(window2TransitionCloseTimerRef.current);
    }
  }, []);

  useEffect(() => {
    if (!deviceId) return;
    const storedSequence = Number(getStorageSync(`${COMMAND_ACK_STORAGE_PREFIX}${deviceId}`) || 0);
    lastAckSequenceRef.current = Number.isFinite(storedSequence) && storedSequence > 0 ? storedSequence : 0;
  }, [deviceId]);

  useEffect(() => {
    setValveLogs(deviceId ? readValveLogCache(deviceId) : []);
  }, [deviceId]);


  useEffect(() => {
    const timer = setInterval(() => {
      const pending = pendingBoardCommandRef.current;
      if (!pending || Date.now() - pending.startedAt < BOARD_COMMAND_ACK_TIMEOUT_MS) return;
      pendingBoardCommandRef.current = null;
      sendingRef.current = false;
      setSending(false);
      if (pending.expectsDeviceTable) {
        syncRequestActiveRef.current = false;
        deviceTableReadPendingRef.current = false;
        syncTransferRef.current = null;
        syncGroupCapacitiesRef.current = null;
        syncDeadlineRef.current = 0;
        setDeviceReportPending(false);
        setSyncingDevices(false);
        setSyncRemaining(0);
      }
      pending.onFailure?.();
      setW2TipText('板端确认超时\n请检查设备在线状态');
      setShowW2ActionTip(true);
      completeWindow2Opening();
    }, 250);
    return () => clearInterval(timer);
  }, [completeWindow2Opening]);

  useEffect(() => {
    if (!deviceId || cacheLoadedDeviceIdRef.current === deviceId) return;
    const cached = readDeviceTableCache(deviceId);
    cacheLoadedDeviceIdRef.current = deviceId;
    deviceTableCacheRef.current = cached;
    deviceTableHashRef.current = cached?.hash ?? null;
    if (cached) {
      setDevices(cached.devices);
      setW2GroupCapacities(cached.groupCapacities);
      setW2GroupNumbers(current => Array.from(new Set([
        ...current,
        ...collectDeviceGroupNumbers(cached.devices),
      ])).sort((a, b) => a - b));
    }
  }, [deviceId]);

  const clearPendingBoardCommand = () => {
    pendingBoardCommandRef.current = null;
    sendingRef.current = false;
    setSending(false);
  };

  const clearDeviceTableVersionFallback = () => {
    if (deviceTableVersionFallbackTimerRef.current !== null) {
      clearTimeout(deviceTableVersionFallbackTimerRef.current);
      deviceTableVersionFallbackTimerRef.current = null;
    }
  };

  const startFullDeviceTableRead = (onFail?: () => void, boardAlreadyResponded = false) => {
    clearDeviceTableVersionFallback();
    pendingAutoAssignRef.current = false;
    deviceTableReadPendingRef.current = true;
    pendingDeleteAddressRef.current = null;
    syncBaselineAddressesRef.current = null;
    syncTransferRef.current = null;
    syncGroupCapacitiesRef.current = null;
    syncRequestActiveRef.current = true;
    syncDeadlineRef.current = Date.now() + DEVICE_SYNC_TIMEOUT_SECONDS * 1000;
    setDeviceReportPending(true);
    setSyncRemaining(0);
    setSyncingDevices(false);
    setShowW2ActionTip(false);
    publishFrame(DEVICE_TABLE_READ_FRAME, '读取设备表', () => {}, false, () => {
      syncRequestActiveRef.current = false;
      deviceTableReadPendingRef.current = false;
      syncTransferRef.current = null;
      syncDeadlineRef.current = 0;
      setDeviceReportPending(false);
      setSyncingDevices(false);
      setSyncRemaining(0);
      onFail?.();
    }, { kind: 'device_sync', command: 'read' }, boardAlreadyResponded);
  };

  useEffect(() => {
    if (normalizedReportedFrame === ROUND_IRRIGATION_ON_FRAME) setIrrigating(true);
    if (normalizedReportedFrame === ROUND_IRRIGATION_OFF_FRAME) setIrrigating(false);
    if (normalizedReportedFrame === NETWORK_NONE_STATE_FRAME) setReportedNetwork('none');
    if (normalizedReportedFrame === NETWORK_ETHERNET_STATE_FRAME) setReportedNetwork('ethernet');
    if (normalizedReportedFrame === NETWORK_WIFI_STATE_FRAME) setReportedNetwork('wifi');
    if (normalizedReportedFrame === NETWORK_4G_STATE_FRAME) setReportedNetwork('4g');
    const rainMatch = normalizedReportedFrame.match(/^AA55F3([0-9A-F]{2})55AA$/);
    const humidityMatch = normalizedReportedFrame.match(/^AA55F4([0-9A-F]{2})55AA$/);
    if (rainMatch) setRainDelayDays(clamp(parseInt(rainMatch[1], 16), 1, 30));
    if (humidityMatch) setHumidityThreshold(clamp(parseInt(humidityMatch[1], 16), 0, 100));

    if (normalizedReportedFrame.startsWith(`${COMMAND_ACK_PREFIX}|`)) {
      const ackParts = normalizedReportedFrame.split('|');
      const ackSequence = Number(ackParts[2]);
      const ackSucceeded = ackParts[3] === 'SUCCEEDED';
      const ackErrorCode = Number(ackParts[4] || 0);
      const reportedKind = (ackParts[5] || '').toLowerCase();
      const reportedCommand = ackParts.slice(6).join('|').toLowerCase();
      const pending = pendingBoardCommandRef.current;
      if (!Number.isFinite(ackSequence) || ackSequence <= 0 || ackSequence <= lastAckSequenceRef.current) return;
      lastAckSequenceRef.current = ackSequence;
      if (deviceId) setStorageSync(`${COMMAND_ACK_STORAGE_PREFIX}${deviceId}`, String(ackSequence));
      if (!pending || ackSequence <= pending.minimumSequence) return;
      if (reportedKind && reportedKind !== pending.kind.toLowerCase()) return;
      if (reportedCommand && reportedCommand !== pending.command.toLowerCase()) return;
       pendingBoardCommandRef.current = null;
       sendingRef.current = false;
       setSending(false);
       if (!ackSucceeded) {
         clearDeviceTableVersionFallback();
         syncRequestActiveRef.current = false;
         deviceTableReadPendingRef.current = false;
         syncTransferRef.current = null;
         syncGroupCapacitiesRef.current = null;
         syncDeadlineRef.current = 0;
         setDeviceReportPending(false);
         setSyncingDevices(false);
         setSyncRemaining(0);
         setW2TipText('板端操作失败\n错误码 ' + (ackErrorCode || 4001));
         setShowW2ActionTip(true);
         pending.onFailure?.();
         completeWindow2Opening();
       } else {
         pending.onSuccess?.();
       }
       return;
      }
    if (!normalizedReportedFrame.startsWith(`${DEVICE_SYNC_PREFIX}|`)) return;
    const parts = normalizedReportedFrame.split('|');
    if (parts.length < 3) return;
    const kind = parts[1];
    const session = parts[2];
    if (kind === 'G' && parts.length === 5) {
      if (!syncRequestActiveRef.current) return;
      const capacityBytes = hexToBytes(parts[3]);
      const capacities = capacityBytes ? decodeGroupCapacities(capacityBytes) : null;
      const dataHash = parseInt(parts[4], 16) >>> 0;
      if (!capacities || !/^[0-9A-F]{256}$/i.test(parts[3]) ||
          !/^[0-9A-F]{8}$/i.test(parts[4]) || Number.isNaN(dataHash)) {
        syncButtonLockRef.current = false;
        syncRequestActiveRef.current = false;
        deviceTableReadPendingRef.current = false;
        syncDeadlineRef.current = 0;
        setDeviceReportPending(false);
        setSyncingDevices(false);
        setW2TipText('设备组容量校验失败\n请重新同步');
        setShowW2ActionTip(true);
        completeWindow2Opening();
        return;
      }
      syncGroupCapacitiesRef.current = capacities;
      setW2GroupCapacities(capacities);
      return;
    }
    if (kind === 'V' && parts.length === 6) {
      if (!syncRequestActiveRef.current) return;
      const versionState = parts[3];
      const count = parseInt(parts[4], 10);
      const dataHash = parseInt(parts[5], 16) >>> 0;
      clearDeviceTableVersionFallback();
      if ((versionState !== 'S' && versionState !== 'C') ||
          Number.isNaN(count) || count < 0 || count > MAX_SYNC_DEVICE_COUNT ||
          !/^[0-9A-F]{8}$/i.test(parts[5])) {
        syncButtonLockRef.current = false;
        syncRequestActiveRef.current = false;
        deviceTableReadPendingRef.current = false;
        syncDeadlineRef.current = 0;
        setDeviceReportPending(false);
        setW2TipText('设备表版本校验失败\n请重新同步');
        setShowW2ActionTip(true);
        completeWindow2Opening();
        return;
      }
      if (versionState === 'S') {
        const cached = deviceTableCacheRef.current;
        if (!cached || cached.hash !== dataHash || cached.devices.length !== count ||
            !isGroupCapacities(cached.groupCapacities) ||
            deviceTableHashRef.current !== dataHash) {
          // The board says its version is unchanged, but the APP cache is not
          // usable. Read the complete table instead of leaving stale rows on
          // screen or reporting a false cache error.
          clearPendingBoardCommand();
          setTimeout(() => {
            if (syncRequestActiveRef.current) {
              startFullDeviceTableRead(completeWindow2Opening, true);
            }
          }, 0);
          return;
        }
        syncRequestActiveRef.current = false;
        if (!pendingBoardCommandRef.current?.expectsDeviceTable) clearPendingBoardCommand();
        deviceTableReadPendingRef.current = false;
        syncDeadlineRef.current = 0;
        syncTransferRef.current = null;
        syncGroupCapacitiesRef.current = null;
        setDeviceReportPending(false);
        setSyncingDevices(false);
        setSyncRemaining(0);
        syncButtonLockRef.current = false;
        setShowW2ActionTip(false);
        completeWindow2Opening();
        return;
      }
      clearPendingBoardCommand();
      setTimeout(() => {
        if (syncRequestActiveRef.current) {
          startFullDeviceTableRead(completeWindow2Opening, true);
        }
      }, 0);
      return;
    }
    if (kind === 'T' && parts.length === 8) {
      if (!syncRequestActiveRef.current) return;
      const count = parseInt(parts[3], 10);
      const tableBytes = hexToBytes(parts[6]);
      const dataHash = parseInt(parts[7], 16) >>> 0;
      const capacities = syncGroupCapacitiesRef.current;
      const decoded = tableBytes && capacities && combinedDeviceTableHash(tableBytes, capacities) === dataHash
        ? decodeDeviceTable(tableBytes, count)
        : null;
      if (!decoded || Number.isNaN(count) || count < 0 || count > MAX_SYNC_DEVICE_COUNT) {
        syncButtonLockRef.current = false;
        syncRequestActiveRef.current = false;
        if (!pendingBoardCommandRef.current?.expectsDeviceTable) clearPendingBoardCommand();
        syncBaselineAddressesRef.current = null;
        syncDeadlineRef.current = 0;
        setDeviceReportPending(false);
        setSyncingDevices(false);
        setW2TipText('设备列表校验失败\n请重新同步');
        setShowW2ActionTip(true);
        completeWindow2Opening();
        return;
      }
      const syncedDevices = normalizeNormalSyncDevices(
        decoded,
        syncBaselineAddressesRef.current,
        pendingAutoAssignRef.current,
      );
      deviceTableHashRef.current = dataHash;
      clearDeviceTableVersionFallback();
      deviceTableCacheRef.current = writeDeviceTableCache(deviceId || '', dataHash, syncedDevices, capacities || {});
      setDevices(syncedDevices);
      setW2GroupNumbers(current => Array.from(new Set([
        ...current,
        ...collectDeviceGroupNumbers(syncedDevices),
      ])).sort((a, b) => a - b));
       syncRequestActiveRef.current = false;
       if (!pendingBoardCommandRef.current?.expectsDeviceTable) clearPendingBoardCommand();
       syncDeadlineRef.current = 0;
       setDeviceReportPending(false);
       setSyncingDevices(false);
       setSyncRemaining(0);
       syncButtonLockRef.current = false;
       if (deviceTableReadPendingRef.current &&
          pendingDeleteAddressRef.current === null && !pendingAutoAssignRef.current) {
        deviceTableReadPendingRef.current = false;
        syncTransferRef.current = null;
        completeWindow2Opening();
        return;
      }
      const deletedAddress = pendingDeleteAddressRef.current;
      if (pendingAutoAssignRef.current) {
        setW2TipText('一键添加完成');
      } else if (deletedAddress !== null) {
        setW2TipText(decoded.some(item => item.address === deletedAddress)
          ? `设备地址 ${deletedAddress}
删除失败`
          : `设备地址 ${deletedAddress}
删除成功`);
      } else {
        const baseline = syncBaselineAddressesRef.current || new Set<number>();
        const appAddedValves = decoded.filter(item => item.type === '电磁阀' && !baseline.has(item.address)).length;
        const appAddedSensors = decoded.filter(item => item.type === '传感器' && !baseline.has(item.address)).length;
        setW2TipText(`此次共添加
电磁阀 数量${appAddedValves}
传感器 数量${appAddedSensors}`);
      }
      setShowW2ActionTip(true);
      completeWindow2Opening();
      pendingDeleteAddressRef.current = null;
      pendingAutoAssignRef.current = false;
      syncBaselineAddressesRef.current = null;
      syncTransferRef.current = null;
      return;
    }
    if (kind === 'S' && parts.length === 6) {
      if (!syncRequestActiveRef.current) return;
      const count = parseInt(parts[3], 10);
      const totalChunks = parseInt(parts[4], 10);
      const dataHash = parseInt(parts[5], 16) >>> 0;
      if (Number.isNaN(count) || count < 0 || count > MAX_SYNC_DEVICE_COUNT ||
          Number.isNaN(totalChunks) || totalChunks < 0 || totalChunks > 9999) {
        return;
      }
      const groupCapacities = syncGroupCapacitiesRef.current;
      if (!groupCapacities) return;
      if (syncTransferRef.current?.session === session) {
        publishSyncAck(session, 10001);
        return;
      }
      syncTransferRef.current = { session, count, totalChunks, dataHash,
        groupCapacities: { ...groupCapacities }, chunks: {}, completed: false };
      if (syncDeadlineRef.current <= Date.now()) {
        syncDeadlineRef.current = Date.now() + DEVICE_SYNC_TIMEOUT_SECONDS * 1000;
      }
      if (!pendingAutoAssignRef.current && !deviceTableReadPendingRef.current) {
        setSyncingDevices(true);
        setShowW2ActionTip(true);
      }
      publishSyncAck(session, 10001);
      return;
    }
    const transfer = syncTransferRef.current;
    if (!transfer || transfer.session !== session) return;
    if (kind === 'C' && parts.length === 7) {
      const index = parseInt(parts[3], 10);
      const total = parseInt(parts[4], 10);
      const payload = parts[5];
      const chunkHash = parseInt(parts[6], 16) >>> 0;
      if (index < 1 || index > transfer.totalChunks || total !== transfer.totalChunks ||
          fnv1a(asciiBytes(payload)) !== chunkHash) {
        return;
      }
      transfer.chunks[index] = payload;
      publishSyncAck(session, index);
      return;
    }
    if (kind === 'E' && parts.length === 6) {
      const count = parseInt(parts[3], 10);
      const total = parseInt(parts[4], 10);
      const dataHash = parseInt(parts[5], 16) >>> 0;
      if (count !== transfer.count || total !== transfer.totalChunks || dataHash !== transfer.dataHash ||
          Object.keys(transfer.chunks).length !== transfer.totalChunks) {
        syncButtonLockRef.current = false;
        syncRequestActiveRef.current = false;
        syncBaselineAddressesRef.current = null;
        syncDeadlineRef.current = 0;
        setDeviceReportPending(false);
        setSyncingDevices(false);
        setShowW2ActionTip(true);
        setW2TipText('设备列表接收不完整\n请重新同步');
        completeWindow2Opening();
        return;
      }
      const tableHex = Array.from({ length: transfer.totalChunks }, (_, index) => transfer.chunks[index + 1] || '').join('');
      const tableBytes = hexToBytes(tableHex);
      const decoded = tableBytes && combinedDeviceTableHash(tableBytes, transfer.groupCapacities) === transfer.dataHash
        ? decodeDeviceTable(tableBytes, transfer.count)
        : null;
      if (!decoded) {
        syncRequestActiveRef.current = false;
        syncBaselineAddressesRef.current = null;
        syncDeadlineRef.current = 0;
        setDeviceReportPending(false);
        setSyncingDevices(false);
        setShowW2ActionTip(true);
        setW2TipText('设备列表校验失败\n请重新同步');
        completeWindow2Opening();
        return;
      }
      const syncedDevices = normalizeNormalSyncDevices(
        decoded,
        syncBaselineAddressesRef.current,
        pendingAutoAssignRef.current,
      );
      transfer.completed = true;
      transfer.decoded = syncedDevices;
      clearDeviceTableVersionFallback();
      deviceTableHashRef.current = dataHash;
      deviceTableCacheRef.current = writeDeviceTableCache(deviceId || '', dataHash, syncedDevices, transfer.groupCapacities);
      setDevices(syncedDevices);
      setW2GroupCapacities(transfer.groupCapacities);
      setW2GroupNumbers(current => Array.from(new Set([
        ...current,
        ...collectDeviceGroupNumbers(syncedDevices),
      ])).sort((a, b) => a - b));
      publishSyncAck(session, 10002);
      return;
    }
    if (kind === 'R' && parts.length === 6) {
      if (!transfer.completed) return;
      syncRequestActiveRef.current = false;
      syncDeadlineRef.current = 0;
      setDeviceReportPending(false);
      setSyncingDevices(false);
      setSyncRemaining(0);
      syncButtonLockRef.current = false;
      if (deviceTableReadPendingRef.current &&
          pendingDeleteAddressRef.current === null && !pendingAutoAssignRef.current) {
        deviceTableReadPendingRef.current = false;
        syncTransferRef.current = null;
        completeWindow2Opening();
        return;
      }
      const deletedAddress = pendingDeleteAddressRef.current;
      if (pendingAutoAssignRef.current) {
        setW2TipText('一键添加完成');
      } else if (deletedAddress !== null) {
        setW2TipText((transfer.decoded || []).some(item => item.address === deletedAddress)
          ? `设备地址 ${deletedAddress}
删除失败`
          : `设备地址 ${deletedAddress}
删除成功`);
      } else {
        const decoded = transfer.decoded || [];
        const baseline = syncBaselineAddressesRef.current || new Set<number>();
        const appAddedValves = decoded.filter(item => item.type === '电磁阀' && !baseline.has(item.address)).length;
        const appAddedSensors = decoded.filter(item => item.type === '传感器' && !baseline.has(item.address)).length;
        setW2TipText(`此次共添加
电磁阀 数量${appAddedValves}
传感器 数量${appAddedSensors}`);
      }
      setShowW2ActionTip(true);
      pendingDeleteAddressRef.current = null;
      pendingAutoAssignRef.current = false;
      syncBaselineAddressesRef.current = null;
      syncTransferRef.current = null;
    }
  }, [normalizedReportedFrame, completeWindow2Opening]);

  useEffect(() => {
    if (!deviceReportPending) return undefined;

    if (syncDeadlineRef.current <= Date.now()) {
      syncDeadlineRef.current = Date.now() + DEVICE_SYNC_TIMEOUT_SECONDS * 1000;
    }
    let timedOut = false;
    const updateRemaining = () => {
      const remaining = Math.max(0, Math.ceil((syncDeadlineRef.current - Date.now()) / 1000));
      const displayDeadline = syncDisplayDeadlineRef.current > 0
        ? syncDisplayDeadlineRef.current
        : syncDeadlineRef.current;
      const displayRemaining = Math.max(0, Math.ceil((displayDeadline - Date.now()) / 1000));
      if (syncingDevices) setSyncRemaining(displayRemaining);
      if (remaining > 0 || timedOut) return;
      timedOut = true;
      clearDeviceTableVersionFallback();
      const autoAssignTimedOut = pendingAutoAssignRef.current;
      const tableReadTimedOut = deviceTableReadPendingRef.current;
       syncRequestActiveRef.current = false;
      clearPendingBoardCommand();
      syncBaselineAddressesRef.current = null;
      pendingDeleteAddressRef.current = null;
      deviceTableReadPendingRef.current = false;
      syncDeadlineRef.current = 0;
      setDeviceReportPending(false);
      setSyncingDevices(false);
      setSyncRemaining(0);
      syncButtonLockRef.current = false;
      setW2TipText(autoAssignTimedOut
        ? '一键添加超时\n请重新同步'
        : tableReadTimedOut
          ? '设备表读取超时\n请重试'
          : '同步超时\n请检查设备连接后重试');
      setShowW2ActionTip(true);
      syncTransferRef.current = null;
      completeWindow2Opening();
      pendingAutoAssignRef.current = false;
    };
    updateRemaining();
    const timer = setInterval(updateRemaining, 1000);

    return () => clearInterval(timer);
  }, [deviceReportPending, syncingDevices]);

  useEffect(() => {
    if (!deviceReportPending && !syncingDevices && !syncRequestActiveRef.current) {
      syncDisplayDeadlineRef.current = 0;
    }
  }, [deviceReportPending, syncingDevices]);

  useEffect(() => {
    if (!deviceReportPending && !syncingDevices && !sending && !syncRequestActiveRef.current) {
      syncButtonLockRef.current = false;
    }
  }, [deviceReportPending, syncingDevices, sending]);

  const currentNetworkIcon = !deviceOnline
    ? networkNoneIcon
    : {
        none: networkNoneIcon,
        ethernet: networkEthernetIcon,
        wifi: networkWifiIcon,
        '4g': network4GIcon,
      }[reportedNetwork];

  const publishFrame = (
    frame: string,
    actionName: string,
    onSuccess?: () => void,
    showSuccessToast = true,
    onFail?: () => void,
    boardCommand?: {
      kind: string;
      command: string;
      expectsDeviceTable?: boolean;
      usesDeviceTableRevision?: boolean;
    },
    allowBoardConfirmedRead = false,
  ) => {
    // A V response is proof that the board is reachable. Its follow-up F502
    // must not be blocked by stale sending/deviceOnline state from F503.
    if (sendingRef.current && !allowBoardConfirmedRead) return;
    if (!deviceId) {
      showToast({ title: '设备ID无效', icon: 'none' });
      return;
    }
    if (boardCommand && !deviceOnline && !allowBoardConfirmedRead) {
      onFail?.();
      showToast({ title: '板端离线，暂不能操作', icon: 'none' });
      return;
    }
    let outboundFrame = frame;
    if (boardCommand?.usesDeviceTableRevision) {
      const baseHash = deviceTableHashRef.current;
      if (baseHash === null) {
        onFail?.();
        showToast({ title: '请先获取最新设备表', icon: 'none' });
        return;
      }
      outboundFrame = appendDeviceTableBaseHash(frame, baseHash);
    }
    if (boardCommand?.expectsDeviceTable) {
      deviceTableReadPendingRef.current = true;
      syncTransferRef.current = null;
      syncGroupCapacitiesRef.current = null;
      syncRequestActiveRef.current = true;
      syncDeadlineRef.current = Date.now() + DEVICE_SYNC_TIMEOUT_SECONDS * 1000;
      setDeviceReportPending(true);
    }
    sendingRef.current = true;
    setSending(true);
    if (boardCommand) {
      pendingBoardCommandRef.current = {
        ...boardCommand,
        actionName,
        showSuccessToast,
        minimumSequence: lastAckSequenceRef.current,
        startedAt: Date.now(),
        onSuccess,
        onFailure: onFail,
      };
    }
    publishDpsBase({
      deviceId,
      dps: { [CJ96_DP_ID]: outboundFrame },
      mode: 1,
      pipelines: [0, 1, 2, 3, 4, 5, 6],
      options: {},
      success: () => {
        if (!boardCommand) {
          sendingRef.current = false;
          setSending(false);
          onSuccess?.();
        }
      },
      fail: (error: any) => {
        sendingRef.current = false;
        setSending(false);
        if (boardCommand && pendingBoardCommandRef.current?.kind === boardCommand.kind &&
            pendingBoardCommandRef.current?.command === boardCommand.command) {
          clearPendingBoardCommand();
        }
        if (boardCommand?.expectsDeviceTable) {
          syncRequestActiveRef.current = false;
          deviceTableReadPendingRef.current = false;
          syncTransferRef.current = null;
          syncGroupCapacitiesRef.current = null;
          syncDeadlineRef.current = 0;
          setDeviceReportPending(false);
          setSyncingDevices(false);
          setSyncRemaining(0);
        }
        onFail?.();
        showToast({ title: formatCallbackValue(error) || '下发失败', icon: 'none' });
      },
    });
  };

  const appendValveLog = useCallback((mode: ValveLogMode, action: '开启' | '关闭') => {
    const currentTime = new Date();
    const timestamp = currentTime.getTime();
    const entry: ValveLogEntry = {
      id: `${timestamp}-${action}`,
      timestamp,
      time: `${currentTime.getFullYear()}-${pad2(currentTime.getMonth() + 1)}-${pad2(currentTime.getDate())} ${pad2(currentTime.getHours())}:${pad2(currentTime.getMinutes())}:${pad2(currentTime.getSeconds())}`,
      week: `周${WEEKDAYS[currentTime.getDay()]}`,
      mode,
      action,
      detail: formatValveLogDetail(devices),
    };
    setValveLogs(current => {
      const next = pruneValveLogs([entry, ...current], timestamp);
      writeValveLogCache(deviceId || '', next);
      return next;
    });
  }, [deviceId, devices]);

  const openRainDialog = () => {
    setRainInput(String(rainDelayDays));
    setActiveDialog('rain');
  };

  const openHumidityDialog = () => {
    setHumidityInput(String(humidityThreshold));
    setActiveDialog('humidity');
  };

  const confirmRainDelay = () => {
    const value = clamp(parseInt(rainInput, 10) || 1, 1, 30);
    publishFrame(makeByteCommandFrame('F3', value), '雨雪延后设置', () => {
      setRainDelayDays(value);
      setRainInput(String(value));
      setActiveDialog(null);
    }, true, undefined, { kind: 'home', command: `rain_delay=${value}` });
  };

  const confirmHumidityThreshold = () => {
    const parsed = parseInt(humidityInput, 10);
    const value = clamp(Number.isNaN(parsed) ? 0 : parsed, 0, 100);
    publishFrame(makeByteCommandFrame('F4', value), '湿度触发设置', () => {
      setHumidityThreshold(value);
      setHumidityInput(String(value));
      setActiveDialog(null);
    }, true, undefined, { kind: 'home', command: `humidity_threshold=${value}` });
  };

  const addDevice = () => {
    const address = clamp(parseInt(newDeviceAddress, 10) || 20, 20, 255);
    if (devices.some(item => item.address === address)) {
      setW2TipText('地址已在列表中');
      setShowW2ActionTip(true);
      return;
    }
    setW2EditingIndex(devices.length);
    setW2EditingAddress(String(address));
    setW2EditingName(newDeviceType === '传感器' ? '传感器' : '电磁阀');
    setW2EditingTypeIndex(newDeviceType === '传感器' ? 1 : 0);
    setW2SelectedGroupNo(1);
    setW2GroupPreviewEnabled(false);
    setW2SelectedPumpIndexes([]);
    setW2SelectedSensorIndexes([]);
    setShowAddDevice(false);
    setShowW2Editor(true);
  };

  const openW2Editor = (index: number) => {
    const item = devices[index];
    if (!item) return;
    setW2EditingIndex(index);
    setW2EditingAddress(String(item.address));
    setW2EditingName(item.name);
    setW2EditingTypeIndex(item.type === '传感器' ? 1 : 0);
    const matchedGroup = item.group.match(/(\d+)/);
    const selectedGroupNo = matchedGroup ? clamp(parseInt(matchedGroup[1], 10) || 1, 1, 128) : 1;
    setW2SelectedGroupNo(selectedGroupNo);
    setW2GroupPreviewEnabled(Boolean(matchedGroup));
    setW2GroupNumbers(current => Array.from(new Set([
      ...current,
      ...extractGroupNumbers(item.group),
      selectedGroupNo,
    ])).sort((a, b) => a - b));
    setW2GroupName(`阀组[${selectedGroupNo}]`);
    setW2SelectedPumpIndexes([]);
    setW2SelectedSensorIndexes([]);
    setShowW2Editor(true);
  };

  const saveW2Editor = () => {
    if (w2EditingIndex === null) return;
    const address = clamp(parseInt(w2EditingAddress, 10) || 20, 20, 255);
    if (devices.some((item, index) => index !== w2EditingIndex && item.address === address)) {
      setW2TipText('地址已存在于列表中');
      setShowW2ActionTip(true);
      return;
    }

    const type: PanelDevice['type'] = w2EditingTypeIndex === 1 ? '传感器' : '电磁阀';
    const typeCode = type === '传感器' ? 3 : 2;
    const name = truncateUtf8Text(
      w2EditingName.trim() || (type === '传感器' ? '传感器' : `电磁阀${address}`),
      19,
    );
    const existing = devices[w2EditingIndex];
    const existingGroup = formatDeviceGroupText(existing?.group || '-');
    const group = type === '传感器' && existing
      ? '='
      : w2GroupPreviewEnabled && w2SelectedGroupNo > 0
        ? String(w2SelectedGroupNo)
        : existingGroup;
    const command = `device_upsert=${address},${typeCode},${group},${name}`;

    publishFrame(makeW2DeviceUpsertCommand(address, type, group, name), '保存设备', () => {
      if (group !== '=' && w2SelectedGroupNo > 0) {
        setW2GroupNumbers(current => Array.from(new Set([...current, w2SelectedGroupNo])).sort((a, b) => a - b));
      }
      w2AutoAssignSnapshotRef.current = null;
      setShowW2Editor(false);
    }, true, undefined, {
      kind: 'home',
      command,
      usesDeviceTableRevision: true,
      expectsDeviceTable: true,
    });
  };

  const cancelW2Editor = () => {
    if (w2AutoAssignSnapshotRef.current) {
      setDevices(w2AutoAssignSnapshotRef.current);
      w2AutoAssignSnapshotRef.current = null;
    }
    setShowW2Editor(false);
  };

  const deleteW2SelectedGroup = (requestedGroupNo = w2SelectedGroupNo) => {
    const targetGroupNo = clamp(requestedGroupNo, 1, 128);
    if (targetGroupNo === 1) {
      setShowW2GroupChoice(false);
      setW2TipText('阀组[1]不能删除');
      setShowW2ActionTip(true);
      setShowW2Editor(true);
      return;
    }

    const frame = makeW2GroupScopeCommand('FC', targetGroupNo);
    publishFrame(frame, '删除阀组', () => {
      const remainingGroups = w2GroupNumbers.filter(groupNo => groupNo !== targetGroupNo);
      setDevices(current => current.map(item => {
        const nextGroup = removeGroupNumber(item.group, targetGroupNo);
        return nextGroup === item.group ? item : { ...item, group: nextGroup };
      }));
      setW2GroupNumbers(remainingGroups);

      const nextSelectedGroupNo = remainingGroups[0] || 1;
      setW2SelectedGroupNo(nextSelectedGroupNo);
      setW2GroupChoiceNo(nextSelectedGroupNo);
      setW2GroupName(`阀组[${nextSelectedGroupNo}]`);
      setW2SelectedPumpIndexes([]);
      setW2SelectedSensorIndexes([]);
      setShowW2GroupChoice(false);
      setShowW2Editor(true);
    }, true, () => setShowW2GroupChoice(true), {
      kind: 'home',
      command: `group_delete=${targetGroupNo}`,
      usesDeviceTableRevision: true,
      expectsDeviceTable: true,
    });
  };

  const closeW2TransientDialogs = () => {
    setShowW2GroupBind(false);
    setShowW2GroupBindScope(false);
    setShowW2GroupChoice(false);
    setShowW2Capacity(false);
    setShowW2Rename(false);
    setShowW2DeviceRename(false);
    setShowW2ClearTip(false);
    setShowW2ActionTip(false);
  };

  const openW2GroupChoice = (mode: W2GroupChoiceMode) => {
    closeW2TransientDialogs();
    const groupNo = clamp(w2SelectedGroupNo, 1, 128);
    setW2GroupChoiceMode(mode);
    setW2GroupChoiceNo(groupNo);
    setW2GroupName(`阀组[${groupNo}]`);
    setShowW2GroupChoice(true);
  };

  const stepW2GroupChoice = (delta: number) => {
    setW2GroupChoiceNo(current => clamp(current + delta, 1, 128));
  };

  const closeW2GroupChoice = () => {
    setShowW2GroupChoice(false);
    setShowW2Editor(true);
  };

  const openW2GroupBind = () => openW2GroupChoice('bind');

  const openW2GroupBindEditor = (allGroups: boolean, groupNo = w2GroupChoiceNo) => {
    const selectedGroupNo = clamp(groupNo, 1, 128);
    setW2SelectedGroupNo(selectedGroupNo);
    setW2GroupName(`阀组[${selectedGroupNo}]`);
    setW2GroupBindAllGroups(allGroups);
    setShowW2GroupChoice(false);
    setShowW2GroupBind(true);
  };

  const confirmW2GroupChoiceCurrent = () => {
    const groupNo = clamp(w2GroupChoiceNo, 1, 128);
    setW2SelectedGroupNo(groupNo);
    setW2GroupName(`阀组[${groupNo}]`);
    if (w2GroupChoiceMode === 'bind') {
      openW2GroupBindEditor(false, groupNo);
    } else if (w2GroupChoiceMode === 'clear') {
      confirmW2Clear(groupNo, false);
    } else if (w2GroupChoiceMode === 'delete') {
      deleteW2SelectedGroup(groupNo);
    } else {
      setShowW2GroupChoice(false);
      setShowW2Rename(true);
    }
  };

  const confirmW2GroupChoiceAll = () => {
    if (w2GroupChoiceMode === 'bind') {
      openW2GroupBindEditor(true, w2GroupChoiceNo);
    } else if (w2GroupChoiceMode === 'clear') {
      confirmW2Clear(w2GroupChoiceNo, true);
    }
  };

  const getW2GroupCapacity = (groupNo: number) => clamp(w2GroupCapacities[groupNo] ?? w2CapacityLast, 1, 3);

  const updateW2CapacityPending = (value: number) => {
    const capacity = clamp(value, 1, 3);
    setW2CapacityPending(capacity);
    if (w2CapacityMode === 'single') {
      setW2GroupCapacities(current => ({ ...current, [w2CapacityGroupNo]: capacity }));
    }
  };

  const selectW2CapacityMode = (mode: W2CapacityMode) => {
    setW2CapacityMode(mode);
    setW2CapacityPending(mode === 'all' ? w2CapacityLast : getW2GroupCapacity(w2CapacityGroupNo));
  };

  const stepW2CapacityGroup = (delta: number) => {
    if (w2CapacityMode !== 'single') return;
    const groupNo = clamp(w2CapacityGroupNo + delta, 1, 128);
    setW2CapacityGroupNo(groupNo);
    setW2CapacityPending(getW2GroupCapacity(groupNo));
  };

  const stepW2CapacityValue = (delta: number) => {
    if (w2CapacityMode !== 'single') return;
    updateW2CapacityPending(w2CapacityPending + delta);
  };

  const openW2Capacity = () => {
    w2CapacityDialogSnapshotRef.current = {
      mode: w2CapacityMode,
      groupNo: w2CapacityGroupNo,
      pending: w2CapacityPending,
      last: w2CapacityLast,
      capacities: { ...w2GroupCapacities },
    };
    const groupNo = clamp(w2SelectedGroupNo, 1, 128);
    setW2CapacityGroupNo(groupNo);
    setW2CapacityPending(w2CapacityMode === 'all'
      ? w2CapacityLast
      : clamp(w2GroupCapacities[groupNo] ?? w2CapacityLast, 1, 3));
    setShowW2Capacity(true);
  };

  const cancelW2Capacity = () => {
    const snapshot = w2CapacityDialogSnapshotRef.current;
    if (snapshot) {
      setW2CapacityMode(snapshot.mode);
      setW2CapacityGroupNo(snapshot.groupNo);
      setW2CapacityPending(snapshot.pending);
      setW2CapacityLast(snapshot.last);
      setW2GroupCapacities(snapshot.capacities);
    }
    w2CapacityDialogSnapshotRef.current = null;
    setShowW2Capacity(false);
  };

  const confirmW2Capacity = () => {
    if (sending) return;
    const capacity = clamp(w2CapacityPending, 1, 3);
    const selectedGroupNo = clamp(w2CapacityGroupNo, 1, 128);
    w2CapacityDialogSnapshotRef.current = null;
    setShowW2Capacity(false);
    pendingAutoAssignRef.current = true;
    publishFrame(makeW2CapacityCommand(w2CapacityMode, selectedGroupNo, capacity), '设置阀组容量', () => {}, true, () => {
      pendingAutoAssignRef.current = false;
      setShowW2Capacity(true);
    }, {
      kind: 'home',
      command: w2CapacityMode === 'all'
        ? `group_auto_assign_all=${capacity}`
        : `group_auto_assign_single=${selectedGroupNo},${capacity}`,
      usesDeviceTableRevision: true,
      expectsDeviceTable: true,
    });
  };

  const openW2Rename = () => openW2GroupChoice('rename');

  const openW2DeviceRename = () => {
    closeW2TransientDialogs();
    setW2DeviceNameDraft(w2EditingName);
    setShowW2DeviceRename(true);
  };

  const openW2ClearTip = () => openW2GroupChoice('clear');

  const closeW2ActionTip = () => {
    if (syncingDevices) return;
    setShowW2ActionTip(false);
  };

  const toggleGroupSelection = (index: number, pump: boolean) => {
    const setter = pump ? setW2SelectedPumpIndexes : setW2SelectedSensorIndexes;
    setter(current => current.includes(index) ? current.filter(item => item !== index) : [...current, index]);
  };

  const confirmW2GroupBind = () => {
    const selectedIndexes = Array.from(new Set([
      ...w2SelectedPumpIndexes,
      ...w2SelectedSensorIndexes,
    ]));
    const selectedAddresses = selectedIndexes
      .map(index => devices[index]?.address)
      .filter((address): address is number => Number.isFinite(address));
    if (!selectedAddresses.length || (!w2GroupBindAllGroups && w2SelectedGroupNo <= 0)) {
      setShowW2GroupBind(false);
      setShowW2Editor(true);
      return;
    }

    const frame = makeW2GroupBindCommand(
      selectedAddresses,
      w2SelectedGroupNo,
      w2GroupBindAllGroups,
    );
    publishFrame(frame, '关联传感器', () => {
      // Update the preview immediately. The subsequent board table report
      // remains the source of truth and replaces this preview.
      setW2GroupPreviewEnabled(true);
      if (!w2GroupBindAllGroups) {
        setW2GroupNumbers(current => Array.from(new Set([...current, w2SelectedGroupNo])).sort((a, b) => a - b));
      }
      setDevices(current => current.map(item => {
        if (!selectedAddresses.includes(item.address)) return item;
        return {
          ...item,
          group: bindPanelDeviceGroup(item, w2SelectedGroupNo, w2GroupBindAllGroups),
          connected: true,
        };
      }));
      setShowW2GroupBind(false);
      setShowW2Editor(true);
    }, true, () => {
      setShowW2GroupBind(true);
    }, {
      kind: 'home',
      command: `group_bind=${w2GroupBindAllGroups ? 255 : w2SelectedGroupNo},${selectedAddresses.join(',')}`,
      usesDeviceTableRevision: true,
      expectsDeviceTable: true,
    });
  };

  const confirmW2Rename = () => {
    const groupNo = clamp(w2SelectedGroupNo, 1, 128);
    const name = w2GroupName.trim() || `阀组[${groupNo}]`;
    const frame = makeW2GroupRenameCommand(groupNo, name);
    publishFrame(frame, '修改名称', () => {
      setW2GroupName(name);
      setShowW2Rename(false);
      setShowW2Editor(true);
    }, true, () => setShowW2Rename(true), {
      kind: 'home',
      command: `group_rename=${groupNo},${name}`,
      usesDeviceTableRevision: true,
      expectsDeviceTable: true,
    });
  };

  const confirmW2Clear = (requestedGroupNo = w2SelectedGroupNo, allGroups = false) => {
    const groupNo = clamp(requestedGroupNo, 1, 128);
    const frame = makeW2GroupScopeCommand('FB', groupNo, allGroups);
    publishFrame(frame, '清空阀组', () => {
      setDevices(current => allGroups
        ? current.map(item => ({ ...item, group: '-' }))
        : current.map(item => {
            const nextGroup = removeGroupNumber(item.group, groupNo);
            return nextGroup === item.group ? item : { ...item, group: nextGroup };
          }));
      setShowW2GroupChoice(false);
      setShowW2ClearTip(false);
      setShowW2Editor(true);
    }, true, () => setShowW2GroupChoice(true), {
      kind: 'home',
      command: `group_clear=${allGroups ? 255 : groupNo}`,
      usesDeviceTableRevision: true,
      expectsDeviceTable: true,
    });
  };

  const deleteDeviceByIndex = (index: number) => {
    const target = devices[index];
    if (!target || target.address < 20 || syncingDevices || sending) return;
    pendingDeleteAddressRef.current = target.address;
    setSyncRemaining(syncDisplayDeadlineRef.current > 0 ? 0 : DEVICE_SYNC_TIMEOUT_SECONDS);
    setSyncingDevices(true);
    setShowW2ActionTip(true);
    publishFrame(makeByteCommandFrame('F6', target.address), '删除设备', () => {}, true, () => {
      pendingDeleteAddressRef.current = null;
    }, {
      kind: 'home',
      command: `device_delete=${target.address}`,
      usesDeviceTableRevision: true,
      expectsDeviceTable: true,
    });
  };

  const requestCurrentDeviceTable = (onFail?: () => void) => {
    if (syncingDevices || sending) return;

    const requestStillActive = deviceReportPending || syncRequestActiveRef.current;
    if (requestStillActive && syncDeadlineRef.current > Date.now()) return;
    if (requestStillActive) {
      // A lost report must not permanently lock Window2 to the old APP cache.
      clearDeviceTableVersionFallback();
      syncRequestActiveRef.current = false;
      deviceTableReadPendingRef.current = false;
      syncTransferRef.current = null;
      syncGroupCapacitiesRef.current = null;
      syncDeadlineRef.current = 0;
      setDeviceReportPending(false);
      setSyncingDevices(false);
      setSyncRemaining(0);
    }

    const cachedHash = deviceTableHashRef.current;
    if (cachedHash === null) {
      // The runtime online hook can lag behind the cloud connection after a
      // real-device preview opens. A read has its own board ACK/data timeout.
      startFullDeviceTableRead(onFail, true);
      return;
    }

    clearDeviceTableVersionFallback();
    pendingAutoAssignRef.current = false;
    deviceTableReadPendingRef.current = true;
    pendingDeleteAddressRef.current = null;
    syncBaselineAddressesRef.current = null;
    syncTransferRef.current = null;
    syncGroupCapacitiesRef.current = null;
    syncRequestActiveRef.current = true;
    syncDeadlineRef.current = Date.now() + DEVICE_SYNC_TIMEOUT_SECONDS * 1000;
    setDeviceReportPending(true);
    setSyncRemaining(0);
    setSyncingDevices(false);
    setShowW2ActionTip(false);
    // Start fallback when F503 is published. V can arrive before the command
    // ACK, and the board ACK sequence can reset after zkgui restarts.
    deviceTableVersionFallbackTimerRef.current = setTimeout(() => {
      deviceTableVersionFallbackTimerRef.current = null;
      if (!syncRequestActiveRef.current || !deviceTableReadPendingRef.current) return;
      clearPendingBoardCommand();
      startFullDeviceTableRead(onFail, true);
    }, DEVICE_TABLE_VERSION_FALLBACK_MS);
    publishFrame(makeDeviceTableVersionFrame(cachedHash), '读取设备表版本', () => {
    }, false, () => {
      clearDeviceTableVersionFallback();
      syncRequestActiveRef.current = false;
      deviceTableReadPendingRef.current = false;
      syncTransferRef.current = null;
      syncDeadlineRef.current = 0;
      setDeviceReportPending(false);
      setSyncingDevices(false);
      setSyncRemaining(0);
      onFail?.();
     }, { kind: 'device_sync', command: 'version' }, true);
  };



  const openWindow2 = () => {
    if (activePage === 'device') return;
    if (!deviceId) {
      setActivePage('device');
      return;
    }

    if (window2TransitionTimerRef.current !== null) {
      clearTimeout(window2TransitionTimerRef.current);
      window2TransitionTimerRef.current = null;
    }
    if (window2TransitionCloseTimerRef.current !== null) {
      clearTimeout(window2TransitionCloseTimerRef.current);
      window2TransitionCloseTimerRef.current = null;
    }

    // Show the cached table immediately. Version validation stays in the
    // background and must not place a modal transition over Window2.
    window2OpeningRef.current = false;
    setWindow2Opening(false);
    setWindow2TransitionClosing(false);
    setActivePage('device');
    requestCurrentDeviceTable();
  };

  const syncDeviceList = () => {
    if (syncButtonLockRef.current || syncingDevices || sending ||
        deviceReportPending || syncRequestActiveRef.current) return;
    syncButtonLockRef.current = true;
    pendingAutoAssignRef.current = false;
    deviceTableReadPendingRef.current = false;
    w2AutoAssignSnapshotRef.current = null;
    setW2GroupPreviewEnabled(false);
    setActivePage('device');
    syncBaselineAddressesRef.current = new Set(devices.map(item => item.address));
    syncTransferRef.current = null;
    syncGroupCapacitiesRef.current = null;
    syncRequestActiveRef.current = true;
    const syncStartedAt = Date.now();
    syncDeadlineRef.current = syncStartedAt + DEVICE_SYNC_TIMEOUT_SECONDS * 1000;
    syncDisplayDeadlineRef.current = syncStartedAt + DEVICE_DISCOVERY_DISPLAY_SECONDS * 1000;
    setDeviceReportPending(true);
    setSyncRemaining(DEVICE_DISCOVERY_DISPLAY_SECONDS);
    setSyncingDevices(true);
    setShowW2ActionTip(true);
    publishFrame(DEVICE_SYNC_FRAME, '设备同步', () => {}, false, () => {
      syncRequestActiveRef.current = false;
      syncBaselineAddressesRef.current = null;
      syncTransferRef.current = null;
      syncGroupCapacitiesRef.current = null;
      syncDeadlineRef.current = 0;
      setDeviceReportPending(false);
      setSyncRemaining(0);
      setSyncingDevices(false);
    }, { kind: 'device_sync', command: 'sync' });
  };

  const updateStartTime = (index: number, value: string) => {
    const clean = value.replace(/[^0-9:]/g, '').slice(0, 5);
    setStartTimes(current => current.map((item, itemIndex) => (itemIndex === index ? clean : item)));
  };

  const updateStartTimePart = (index: number, part: 0 | 1, value: string) => {
    const current = startTimes[index] || '--:--';
    const values = current.split(':');
    const digits = value.replace(/\D/g, '').slice(0, 2);
    if (!digits) {
      updateStartTime(index, '--:--');
      return;
    }
    const limit = part === 0 ? 23 : 59;
    values[part] = String(Math.min(limit, Number(digits)));
    const otherPart = part === 0 ? 1 : 0;
    if (!values[otherPart] || !values[otherPart].replace(/\D/g, '')) {
      values[otherPart] = '00';
    }
    updateStartTime(index, `${values[0] || '--'}:${values[1] || '--'}`);
  };

  const formatPlanTime = (value: string) => {
    const [hour = '', minute = ''] = value.split(':');
    const cleanHour = hour.replace(/\D/g, '');
    const cleanMinute = minute.replace(/\D/g, '');
    if (!cleanHour || !cleanMinute) {
      return ['--', '--'] as const;
    }
    return [cleanHour.padStart(2, '0'), cleanMinute.padStart(2, '0')] as const;
  };

  const toggleWeekday = (day: number) => {
    setWeekMode(true);
    setIntervalDays('-');
    setSelectedWeekdays(current =>
      current.includes(day) ? current.filter(item => item !== day) : [...current, day]
    );
  };

  const selectWeekMode = () => {
    setWeekMode(true);
    setIntervalDays('-');
  };

  const selectIntervalMode = () => {
    setWeekMode(false);
    setSelectedWeekdays([]);
  };

  const toggleEveryDay = () => {
    setWeekMode(true);
    setIntervalDays('-');
    setSelectedWeekdays(current => current.length === WEEKDAY_OPTIONS.length
      ? []
      : WEEKDAY_OPTIONS.map((_, index) => index));
  };

  const toggleProgramEnabled = () => {
    if (programEnabled) {
      setProgramEnabled(false);
      return;
    }

    const hasStartTime = startTimes.some(value => {
      const [hour = '', minute = ''] = value.split(':');
      return /^\d{1,2}$/.test(hour) && /^\d{1,2}$/.test(minute);
    });
    if (!hasStartTime) {
      showToast({ title: '请选择开启时间', icon: 'none' });
      return;
    }
    if (weekMode ? selectedWeekdays.length === 0 : !/\d/.test(intervalDays)) {
      showToast({ title: weekMode ? '请设置星期/隔天模式' : '请设置间隔模式', icon: 'none' });
      return;
    }
    if (!groupDurations.some(value => Number(value) > 0)) {
      showToast({ title: '请设置阀组运行时间', icon: 'none' });
      return;
    }
    setProgramEnabled(true);
  };

  const toggleCycleEnabled = () => {
    if (cycleEnabled) {
      setCycleEnabled(false);
      return;
    }

    setShowCycleWindow(true);
  };

  const confirmCycleSettings = () => {
    if (!cycleRanges.some(range => cycleRangeSeconds(range) > 0)) {
      showToast({ title: '请至少设置一个启动时间', icon: 'none' });
      return;
    }
    if (cycleDurationSeconds(cycleIrrigation) <= 0) {
      showToast({ title: '请设置开始时间', icon: 'none' });
      return;
    }
    if (cycleDurationSeconds(cycleSoak) <= 0) {
      showToast({ title: '请设置关闭时间', icon: 'none' });
      return;
    }

    setCycleEnabled(true);
    setShowCycleWindow(false);
  };

  const normalizeCycleValue = (value: string, maxValue: number, maxLength = 2) => {
    const digits = value.replace(/\D/g, '').slice(0, maxLength);
    return digits ? String(Math.min(maxValue, Number(digits))) : '';
  };

  const formatCycleTimeValue = (value: string) => value ? value.padStart(2, '0') : '';

  const cycleRangeSeconds = (range: CycleRange) => {
    if (!range.startHour || !range.endHour) return 0;
    const start = Number(range.startHour) * 60 + Number(range.startMinute || 0);
    const end = Number(range.endHour) * 60 + Number(range.endMinute || 0);
    return end > start ? (end - start) * 60 : 0;
  };

  const cycleDurationSeconds = (duration: CycleDuration) => (
    (Number(duration.hour) * 60 + Number(duration.minute)) * 60 + Number(duration.second)
  );

  const secondsToCycleDuration = (seconds: number): CycleDuration => {
    const bounded = Math.max(0, seconds);
    const hour = Math.min(99, Math.floor(bounded / 3600));
    const remainder = bounded % 3600;
    return {
      hour: String(hour),
      minute: String(Math.floor(remainder / 60)),
      second: String(remainder % 60),
    };
  };

  const autoFillCycleDurations = (ranges: CycleRange[], count: string) => {
    const totalSeconds = ranges.reduce((sum, range) => sum + cycleRangeSeconds(range), 0);
    const countValue = Number(count);
    if (totalSeconds <= 0 || !Number.isFinite(countValue) || countValue <= 0) return null;

    const oneCycleSeconds = Math.floor(totalSeconds / countValue);
    if (oneCycleSeconds <= 0) return null;
    const irrigationSeconds = Math.floor(oneCycleSeconds / 2);
    return {
      irrigation: secondsToCycleDuration(irrigationSeconds),
      soak: secondsToCycleDuration(oneCycleSeconds - irrigationSeconds),
    };
  };

  const cycleCountFromDurations = (
    ranges: CycleRange[],
    irrigation: CycleDuration,
    soak: CycleDuration
  ) => {
    const totalSeconds = ranges.reduce((sum, range) => sum + cycleRangeSeconds(range), 0);
    const oneCycleSeconds = cycleDurationSeconds(irrigation) + cycleDurationSeconds(soak);
    if (totalSeconds <= 0 || oneCycleSeconds <= 0) return null;
    return String(Math.min(99, Math.max(1, Math.floor(totalSeconds / oneCycleSeconds))));
  };

  const updateCycleRange = (index: number, field: keyof CycleRange, value: string) => {
    const maxValue = field === 'startHour' || field === 'endHour' ? 23 : 59;
    const nextRanges = cycleRanges.map((range, rangeIndex) => {
      if (rangeIndex !== index) return range;
      const next = { ...range, [field]: normalizeCycleValue(value, maxValue) };
      if (next[field] && field === 'startHour' && !next.startMinute) next.startMinute = '0';
      if (next[field] && field === 'endHour' && !next.endMinute) next.endMinute = '0';
      return next;
    });
    setCycleRanges(nextRanges);
    const hasConfiguredRange = nextRanges.some(range => cycleRangeSeconds(range) > 0);
    const nextCount = !cycleCountInitialized && hasConfiguredRange ? '3' : cycleCount;
    if (!cycleCountInitialized && hasConfiguredRange) {
      setCycleCount('3');
      setCycleCountInitialized(true);
    }
    const durations = autoFillCycleDurations(nextRanges, nextCount);
    if (durations) {
      setCycleIrrigation(durations.irrigation);
      setCycleSoak(durations.soak);
    }
  };

  const updateCycleDuration = (
    target: 'irrigation' | 'soak',
    field: keyof CycleDuration,
    value: string
  ) => {
    const nextDuration = {
      ...(target === 'irrigation' ? cycleIrrigation : cycleSoak),
      [field]: normalizeCycleValue(value, field === 'hour' ? 99 : 59),
    };
    const nextIrrigation = target === 'irrigation' ? nextDuration : cycleIrrigation;
    const nextSoak = target === 'soak' ? nextDuration : cycleSoak;
    if (target === 'irrigation') setCycleIrrigation(nextIrrigation);
    else setCycleSoak(nextSoak);
    setCycleCountInitialized(true);

    const nextCount = cycleCountFromDurations(cycleRanges, nextIrrigation, nextSoak);
    if (nextCount) setCycleCount(nextCount);
  };

  const updateCycleCount = (value: string) => {
    const normalized = normalizeCycleValue(value, 99);
    const nextCount = normalized && Number(normalized) > 0 ? normalized : '';
    setCycleCount(nextCount);
    if (nextCount) setCycleCountInitialized(true);
    if (!nextCount) return;

    const durations = autoFillCycleDurations(cycleRanges, nextCount);
    if (durations) {
      setCycleIrrigation(durations.irrigation);
      setCycleSoak(durations.soak);
    }
  };

  const toggleIndexedState = (
    setter: React.Dispatch<React.SetStateAction<boolean[]>>,
    index: number
  ) => {
    setter(current => current.map((value, itemIndex) => (itemIndex === index ? !value : value)));
  };

  const window4OutputDevices = devices.filter(item => item.type === '水泵' || item.type === '电磁阀');
  const window4SensorDevices = devices
    .filter(item => item.type === '传感器')
    .sort((left, right) => left.address - right.address);
  const window4GroupNumbers = w2GroupNumbers.length ? w2GroupNumbers : [1, 2, 3, 4];

  const toggleWindow4Device = (address: number, currentState: boolean) => {
    if (sending) return;
    const enabled = !currentState;
    publishFrame(
      makeWindow4StateFrame(WINDOW4_DEVICE_STATE_COMMAND, address, enabled),
      enabled ? '设备开启' : '设备关闭',
      () => setDevices(current => current.map(item => item.address === address
        ? { ...item, stateKnown: true, state: enabled, connected: true, status: enabled ? '打开' : '关闭' }
        : item)),
      true,
      undefined,
      { kind: 'home', command: `device_state=${address},${enabled ? 1 : 0}` },
    );
  };

  const toggleWindow4Group = (index: number, groupNo: number) => {
    if (sending) return;
    const enabled = !Boolean(groupTestStates[index]);
    publishFrame(
      makeWindow4StateFrame(WINDOW4_GROUP_STATE_COMMAND, groupNo, enabled),
      enabled ? '阀组开启' : '阀组关闭',
      () => setGroupTestStates(current => current.map((value, itemIndex) => itemIndex === index ? enabled : value)),
      true,
      undefined,
      { kind: 'home', command: `group_state=${groupNo},${enabled ? 1 : 0}` },
    );
  };

  const confirmRoundIrrigation = () => {
    setShowRoundConfirm(false);
    publishFrame(ROUND_IRRIGATION_ON_FRAME, '轮灌开启', () => {
      setIrrigating(true);
      appendValveLog('手动灌溉', '开启');
    }, true, undefined, {
      kind: 'round_irrigation',
      command: 'on',
    });
  };

  const renderDeviceRow = (row: DeviceListRow) => {
    if (row.kind === 'empty') {
      return (
        <View className={styles.deviceRow} key="w2-empty-row">
          <View className={`${styles.deviceCell} ${styles.deviceAddressCell}`} onClick={() => {
            setNewDeviceAddress('20');
            setShowAddDevice(true);
          }}>
            <Text className={styles.deviceLinkText}>点击添加</Text>
          </View>
          <View className={`${styles.deviceCell} ${styles.deviceNameCell}`} />
          <View className={`${styles.deviceCell} ${styles.deviceTypeCell}`} />
          <View className={`${styles.deviceCell} ${styles.deviceGroupCell}`} />
          <View className={`${styles.deviceCell} ${styles.deviceStatusCell}`} />
          <View
            className={`${styles.deviceCell} ${styles.deviceOperationCell}`}
            onClick={syncingDevices ? undefined : syncDeviceList}
          >
            <Text className={styles.deviceLinkText}>{syncingDevices ? '同步中' : '同步'}</Text>
          </View>
        </View>
      );
    }

    const { device: item, index } = row;
    const canDelete = index >= DEFAULT_DEVICE_COUNT && item.address >= 20;
    const canOpenEditor = index >= DEFAULT_DEVICE_COUNT;
    const openEditor = canOpenEditor ? () => openW2Editor(index) : undefined;

    return (
      <View
        className={`${styles.deviceRow} ${canOpenEditor ? styles.deviceRowEditable : ''}`}
        key={`${item.address}-${item.type}-${index}`}
      >
        <View className={`${styles.deviceCell} ${styles.deviceAddressCell}`} onClick={openEditor}><Text>{item.address}</Text></View>
        <View className={`${styles.deviceCell} ${styles.deviceNameCell}`} onClick={openEditor}><Text>{item.name}</Text></View>
        <View className={`${styles.deviceCell} ${styles.deviceTypeCell}`} onClick={openEditor}><Text>{item.type}</Text></View>
        <View className={`${styles.deviceCell} ${styles.deviceGroupCell}`} onClick={openEditor}><Text>{formatDeviceGroupText(item.group)}</Text></View>
        <View className={`${styles.deviceCell} ${styles.deviceStatusCell}`} onClick={openEditor}>
          <Text className={item.connected ? styles.onlineText : styles.offlineText}>
            {item.status || (item.connected ? '已连接' : '未连接')}
          </Text>
        </View>
        <View
          className={`${styles.deviceCell} ${styles.deviceOperationCell}`}
          onClick={canDelete ? () => deleteDeviceByIndex(index) : undefined}
        >
          <Text className={canDelete ? styles.deviceLinkText : styles.deviceBlankText}>
            {canDelete ? '删除' : ''}
          </Text>
        </View>
      </View>
    );
  };

  const renderDevicePage = () => {
    const rows: DeviceListRow[] = [
      ...devices
        .map((deviceItem, index) => ({ kind: 'device' as const, device: deviceItem, index }))
        .sort((left, right) => left.device.address - right.device.address || left.index - right.index),
      { kind: 'empty' as const, index: devices.length },
    ];
    const columnClasses = [
      styles.deviceAddressCell,
      styles.deviceNameCell,
      styles.deviceTypeCell,
      styles.deviceGroupCell,
      styles.deviceStatusCell,
      styles.deviceOperationCell,
    ];

    return (
      <View className={`${styles.pagePanel} ${styles.devicePage}`}>
        <Image className={styles.devicePageBackground} src={deviceListBackground} mode="scaleToFill" />
        <View className={`${styles.deviceRow} ${styles.deviceHeaderRow}`}>
          {DEVICE_LIST_HEADERS.map((title, index) => (
            <View
              className={`${styles.deviceCell} ${columnClasses[index]}`}
              key={title}
            >
              <Text>{title}</Text>
            </View>
          ))}
        </View>
      <ScrollView className={styles.deviceList} scrollY enhanced showScrollbar>
        <View className={styles.deviceListContent}>
          {rows.map(renderDeviceRow)}
        </View>
      </ScrollView>
      </View>
    );
  };

  const renderPlanPage = () => (
    <View className={`${styles.pagePanel} ${styles.planPage}`}>
      <ScrollView className={styles.planScroller} scrollY enhanced showScrollbar>
        <View className={styles.planContent}>
          <View className={`${styles.planRegion} ${styles.planRegionCycle}`}>
            <Image className={styles.regionBackground} src={window3Region1} mode="scaleToFill" />
            <Image
              className={styles.planCycleIcon}
              src={window3SprayProgramIcon}
              mode="aspectFit"
              onClick={() => setShowCycleWindow(true)}
            />
            <Image
              className={styles.planCycleSwitch}
              src={cycleEnabled ? window3CycleToggleOnIcon : window3CycleToggleOffIcon}
              mode="aspectFit"
              onClick={toggleCycleEnabled}
            />
          </View>

          <View className={`${styles.planRegion} ${styles.planRegionProgram}`}>
            <Image className={styles.regionBackground} src={window3Region2} mode="scaleToFill" />
            <Image className={styles.planProgramIcon} src={window3ProgramIcon} mode="aspectFit" />
            <View className={styles.planProgramPrev} onClick={() => setProgramIndex(value => Math.max(1, value - 1))}><Text>‹</Text></View>
            <Text className={styles.planProgramName}>程序{programIndex}</Text>
            <View className={styles.planProgramNext} onClick={() => setProgramIndex(value => Math.min(16, value + 1))}><Text>›</Text></View>
          </View>

          <View className={`${styles.planRegion} ${styles.planRegionEnable}`}>
            <Image className={styles.regionBackground} src={window3Region3} mode="scaleToFill" />
            <Image className={styles.planEnableIcon} src={window3EnableIcon} mode="aspectFit" />
            <Image
              className={styles.planCycleSwitch}
              src={programEnabled ? window3CycleToggleOnIcon : window3CycleToggleOffIcon}
              mode="aspectFit"
              onClick={toggleProgramEnabled}
            />
          </View>

          <View className={`${styles.planRegion} ${styles.planRegionStart}`}>
            <Image className={styles.regionBackground} src={window3Region4} mode="scaleToFill" />
            <Image className={styles.planStartIcon} src={window3StartTimeIcon} mode="aspectFit" />
            <View className={styles.planStartTimeGrid}>
              {startTimes.map((value, index) => (
                <View className={styles.planTimeEditor} key={`time-${index}`}>
                  <View className={styles.planTimeClock}>
                    <Image className={styles.planTimeClockIcon} src={WINDOW3_ALARM_ICONS[index]} mode="aspectFit" />
                  </View>
                  <View className={styles.planTimePart}>
                    <Text>{formatPlanTime(value)[0]}</Text>
                    <Input
                      className={styles.planTimeNativeInput}
                      type="number"
                      maxLength={2}
                      value={value.split(':')[0].replace(/\D/g, '')}
                      onInput={event => updateStartTimePart(index, 0, String(event.detail.value))}
                    />
                  </View>
                  <Text className={styles.planTimeColon}>:</Text>
                  <View className={styles.planTimePart}>
                    <Text>{formatPlanTime(value)[1]}</Text>
                    <Input
                      className={styles.planTimeNativeInput}
                      type="number"
                      maxLength={2}
                      value={value.split(':')[1].replace(/\D/g, '')}
                      onInput={event => updateStartTimePart(index, 1, String(event.detail.value))}
                    />
                  </View>
                </View>
              ))}
            </View>
          </View>

          <View className={`${styles.planRegion} ${styles.planRegionWeek}`}>
            <Image className={styles.regionBackground} src={window3Region5} mode="scaleToFill" />
            <Image className={styles.planModeIcon} src={window3WeekModeIcon} mode="aspectFit" />
            <View className={styles.planModeHeader} onClick={selectWeekMode}>
              <View className={`${styles.planModeRadio} ${weekMode ? styles.planModeRadioSelected : ''}`}><Text>{weekMode ? '✓' : ''}</Text></View>
              <Text className={styles.planModeTitle}>星期模式</Text>
            </View>
            <View className={styles.planWeekdayGrid}>
              <View className={styles.planEveryDay} onClick={toggleEveryDay}>
                <View className={`${styles.planDayRadio} ${selectedWeekdays.length === 7 ? styles.planDayRadioSelected : ''}`}>
                  <Text>{selectedWeekdays.length === 7 ? '✓' : ''}</Text>
                </View>
                <Text>每天</Text>
              </View>
              {WEEKDAY_OPTIONS.map((day, index) => (
                <View
                  key={day}
                  className={`${styles.planWeekdayButton} ${selectedWeekdays.includes(index) ? styles.planWeekdaySelected : ''}`}
                  onClick={() => toggleWeekday(index)}
                ><View className={styles.planDayRadio}><Text>{selectedWeekdays.includes(index) ? '✓' : ''}</Text></View><Text>星期{day}</Text></View>
              ))}
            </View>
          </View>

          <View className={`${styles.planRegion} ${styles.planRegionInterval}`}>
            <Image className={styles.regionBackground} src={window3Region6} mode="scaleToFill" />
            <Image className={styles.planModeIcon} src={window3IntervalModeIcon} mode="aspectFit" />
            <View className={styles.planModeHeader} onClick={selectIntervalMode}>
              <View className={`${styles.planModeRadio} ${!weekMode ? styles.planModeRadioSelected : ''}`}><Text>{!weekMode ? '✓' : ''}</Text></View>
              <Text className={styles.planModeTitle}>隔天模式</Text>
            </View>
            <Image className={styles.planIntervalSymbol} src={window3IntervalIcon} mode="aspectFit" />
            <View className={styles.planIntervalInput}>
              <Text>{intervalDays || '-'}</Text>
              <Input
                className={styles.planIntervalNativeInput}
                type="number"
                maxLength={2}
                value={intervalDays.replace(/\D/g, '')}
                onInput={event => {
                  selectIntervalMode();
                  const digits = String(event.detail.value).replace(/\D/g, '').slice(0, 2);
                  setIntervalDays(digits ? String(Math.min(99, Math.max(1, Number(digits)))) : '-');
                }}
              />
            </View>
          </View>

          <View className={`${styles.planRegion} ${styles.planRegionDuration}`}>
            <Image className={styles.regionBackground} src={window3Region7} mode="scaleToFill" />
            <View className={styles.planValveGroupList}>
              {[1, 2].map(index => <Text className={styles.planValveGroupRow} key={index}>空阀组</Text>)}
            </View>
          </View>
        </View>
      </ScrollView>
    </View>
  );

  const renderCycleTimeBox = (
    className: string,
    value: string,
    onInput: (value: string) => void,
    inputKey: string
  ) => (
    <View className={className}>
      <Text>{formatCycleTimeValue(value)}{activeCycleInput === inputKey && <Text className={styles.cycleInputCaret}>|</Text>}</Text>
      <Input
        className={styles.cycleNativeInput}
        type="number"
        maxLength={2}
        value={value}
        onFocus={() => setActiveCycleInput(inputKey)}
        onBlur={() => setActiveCycleInput(current => current === inputKey ? null : current)}
        onInput={event => onInput(String(event.detail.value))}
      />
    </View>
  );

  const renderCycleTimeRow = (index: number, topClass: string) => (
    <View className={`${styles.cycleTimeRow} ${styles[topClass]}`} key={`cycle-row-${index}`}>
      <Image className={styles.cycleAlarmIcon} src={WINDOW3_ALARM_ICONS[index]} mode="aspectFit" />
      {renderCycleTimeBox(styles.cycleStartHour, cycleRanges[index].startHour, value => updateCycleRange(index, 'startHour', value), `cycle-range-${index}-start-hour`)}
      <Image className={styles.cycleColon} src={cycleColon} mode="scaleToFill" />
      {renderCycleTimeBox(styles.cycleStartMinute, cycleRanges[index].startMinute, value => updateCycleRange(index, 'startMinute', value), `cycle-range-${index}-start-minute`)}
      <Text className={styles.cycleRangeMark}>~</Text>
      {renderCycleTimeBox(styles.cycleEndHour, cycleRanges[index].endHour, value => updateCycleRange(index, 'endHour', value), `cycle-range-${index}-end-hour`)}
      <Image className={styles.cycleEndColon} src={cycleColon} mode="scaleToFill" />
      {renderCycleTimeBox(styles.cycleEndMinute, cycleRanges[index].endMinute, value => updateCycleRange(index, 'endMinute', value), `cycle-range-${index}-end-minute`)}
    </View>
  );

  const renderCycleNumberBox = (
    className: string,
    value: string,
    onInput: (value: string) => void,
    maxLength = 2,
    wide = false,
    padToTwoDigits = true,
    inputKey = className
  ) => (
    <View className={`${styles.cycleNumberBox} ${wide ? styles.cycleNumberBoxWide : ''} ${styles[className]}`}>
      <Text>{padToTwoDigits ? formatCycleTimeValue(value) : value}{activeCycleInput === inputKey && <Text className={styles.cycleInputCaret}>|</Text>}</Text>
      <Input
        className={styles.cycleNativeInput}
        type="number"
        maxLength={maxLength}
        value={value}
        onFocus={() => setActiveCycleInput(inputKey)}
        onBlur={() => setActiveCycleInput(current => current === inputKey ? null : current)}
        onInput={event => onInput(String(event.detail.value))}
      />
    </View>
  );

  const renderCyclePage = () => (
    <View className={`${styles.pagePanel} ${styles.cyclePage}`}>
      <Image className={styles.cycleLeftRegion} src={cycleRegionLeft} mode="scaleToFill" />
      <Image className={styles.cycleRightRegion} src={cycleRegionRight} mode="scaleToFill" />
      <Text className={styles.cycleStartTitle}>启动时间</Text>
      {renderCycleTimeRow(0, 'cycleFirstRow')}
      {renderCycleTimeRow(1, 'cycleSecondRow')}
      <Text className={styles.cycleCountLabel}>喷雾次数</Text>
      {renderCycleNumberBox('cycleCountValue', cycleCount, updateCycleCount, 2, true, false)}
      <Text className={styles.cycleCountUnit}>次</Text>

      <Text className={styles.cycleRightLabelStart}>开始时间</Text>
      {renderCycleNumberBox('cycleIrrigationHour', cycleIrrigation.hour, value => updateCycleDuration('irrigation', 'hour', value))}
      <Text className={styles.cycleHourUnit}>时</Text>
      {renderCycleNumberBox('cycleIrrigationMinute', cycleIrrigation.minute, value => updateCycleDuration('irrigation', 'minute', value))}
      <Text className={styles.cycleMinuteUnit}>分</Text>
      {renderCycleNumberBox('cycleIrrigationSecond', cycleIrrigation.second, value => updateCycleDuration('irrigation', 'second', value))}
      <Text className={styles.cycleSecondUnit}>秒</Text>

      <Text className={styles.cycleRightLabelEnd}>关闭时间</Text>
      {renderCycleNumberBox('cycleSoakHour', cycleSoak.hour, value => updateCycleDuration('soak', 'hour', value))}
      <Text className={styles.cycleEndHourUnit}>时</Text>
      {renderCycleNumberBox('cycleSoakMinute', cycleSoak.minute, value => updateCycleDuration('soak', 'minute', value))}
      <Text className={styles.cycleEndMinuteUnit}>分</Text>
      {renderCycleNumberBox('cycleSoakSecond', cycleSoak.second, value => updateCycleDuration('soak', 'second', value))}
      <Text className={styles.cycleEndSecondUnit}>秒</Text>

      <Text className={styles.cycleIntervalLabel}>喷雾间隔</Text>
      {renderCycleNumberBox('cycleIntervalValue', cycleIntervalDays, value => setCycleIntervalDays(normalizeCycleValue(value, 255, 3)), 3, true, false)}
      <Text className={styles.cycleIntervalUnit}>天 每天</Text>

      <Image className={styles.cycleCancelButton} src={cycleCancelImage} mode="scaleToFill" onClick={() => setShowCycleWindow(false)} />
      <Image
        className={styles.cycleConfirmButton}
        src={cycleConfirmImage}
        mode="scaleToFill"
        onClick={confirmCycleSettings}
      />
    </View>
  );

  const renderRoundNumberBox = (
    value: string,
    onInput: (value: string) => void,
    inputKey: string
  ) => (
    <View className={styles.roundNumberBox}>
      <Text>{value}{activeCycleInput === inputKey && <Text className={styles.cycleInputCaret}>|</Text>}</Text>
      <Input
        className={styles.cycleNativeInput}
        type="number"
        maxLength={2}
        value={value}
        onFocus={() => setActiveCycleInput(inputKey)}
        onBlur={(event: any) => {
          setActiveCycleInput(current => current === inputKey ? null : current);
          if (!String(event?.detail?.value || '').replace(/\D/g, '')) onInput('0');
        }}
        onInput={event => onInput(String(event.detail.value).replace(/\D/g, ''))}
      />
    </View>
  );

  const renderTestPage = () => (
    <View className={`${styles.pagePanel} ${styles.testPage}`}>
      <View className={styles.testColumn}>
        <Image className={styles.regionBackground} src={window4Region1} mode="scaleToFill" />
        <Text className={styles.testTitle}>设备测试</Text>
        <View className={styles.testHeader}><Text>名称</Text><Text>编号</Text><Text>操作</Text></View>
        {window4OutputDevices.map(item => (
          <View className={styles.testRow} key={`device-test-${item.address}`}>
            <Text>{item.name}</Text><Text>{item.address}</Text>
            <Image className={styles.testToggle} src={item.state ? window3CycleToggleOnIcon : window3CycleToggleOffIcon} mode="scaleToFill" onClick={() => toggleWindow4Device(item.address, item.state)} />
          </View>
        ))}
      </View>
      <View className={`${styles.testColumn} ${styles.groupColumn}`}>
        <Image className={styles.regionBackground} src={window4Region2} mode="scaleToFill" />
        <Text className={styles.testTitle}>阀组测试</Text>
        <View className={styles.testHeader}><Text>编号</Text><Text>名称</Text><Text>操作</Text></View>
        <ScrollView className={styles.groupTestList} scrollY enhanced>
          {window4GroupNumbers.map((groupNo, index) => (
            <View className={styles.testRow} key={`group-test-${groupNo}`}>
              <Text>{groupNo}</Text><Text>阀组[{groupNo}]</Text>
              <Image className={styles.testToggle} src={groupTestStates[index] ? window3CycleToggleOnIcon : window3CycleToggleOffIcon} mode="scaleToFill" onClick={() => toggleWindow4Group(index, groupNo)} />
            </View>
          ))}
        </ScrollView>
        <View className={styles.roundControls}>
          {renderRoundNumberBox(roundMinutes, setRoundMinutes, 'roundMinutes')}
          <Text className={styles.roundUnit}>分</Text>
          {renderRoundNumberBox(roundSeconds, setRoundSeconds, 'roundSeconds')}
          <Text className={styles.roundUnit}>秒</Text>
          <Image className={styles.roundToggle} src={irrigating ? window3CycleToggleOnIcon : window3CycleToggleOffIcon} mode="scaleToFill" onClick={() => irrigating ? publishFrame(ROUND_IRRIGATION_OFF_FRAME, '轮灌关闭', () => {
            setIrrigating(false);
            appendValveLog('手动', '关闭');
          }, true, undefined, { kind: 'round_irrigation', command: 'off' }) : setShowRoundConfirm(true)} />
        </View>
      </View>
      <View className={`${styles.testColumn} ${styles.sensorColumn}`}>
        <Image className={styles.regionBackground} src={window4Region3} mode="scaleToFill" />
        <Text className={styles.testTitle}>传感器数据</Text>
        <View className={styles.testHeader}><Text>编号</Text><Text>名称</Text><Text>数据</Text></View>
        {window4SensorDevices.map(item => (
          <View className={styles.sensorDataRow} key={`sensor-test-${item.address}`}><Text className={styles.sensorCell}>{item.address}</Text><Text className={styles.sensorCell}>{item.name}</Text><Text className={styles.sensorCell}>{item.status || '-'}</Text></View>
        ))}
      </View>
    </View>
  );

  const renderValveLogPage = () => (
    <View className={`${styles.pagePanel} ${styles.logPage}`}>
      <View className={styles.logCard}>
        <Text className={styles.logTitle}>灌溉日志</Text>
        <ScrollView className={styles.logScroller} scrollY enhanced showScrollbar>
          {valveLogs.length ? valveLogs.map(entry => (
            <View className={styles.logRow} key={entry.id}>
              <Text className={styles.logTime}>{entry.time}</Text>
              <Text className={styles.logWeek}>{entry.week}</Text>
              <Text className={styles.logMode}>{entry.mode}</Text>
              <Text className={styles.logAction}>{entry.action}</Text>
              <Text className={styles.logDetail}>{entry.detail}</Text>
            </View>
          )) : (
            <View className={styles.logEmpty}>
              <Text>暂无灌溉日志</Text>
            </View>
          )}
        </ScrollView>
      </View>
    </View>
  );

  return (
    <View className={styles.stage}>
      <View className={styles.canvas}>
        <View className={styles.header}>
          <Text className={styles.panelVersion}>版本 {PANEL_VERSION}</Text>
          <Image className={styles.brand} src={brandImage} mode="aspectFit" />
          <Image className={styles.networkIcon} src={currentNetworkIcon} mode="aspectFit" />
          <Text className={styles.clock}>
            {pad2(now.getHours())}:{pad2(now.getMinutes())}:{pad2(now.getSeconds())}
          </Text>
          <Text className={styles.date}>
            {now.getFullYear()}/{now.getMonth() + 1}/{now.getDate()}
          </Text>
          <View className={styles.week}>
            <Text>周</Text>
            <Text>{WEEKDAYS[now.getDay()]}</Text>
          </View>
        </View>

        {activePage === 'overview' && <View className={styles.contentBand}>
          <View className={`${styles.card} ${styles.statusCard}`}>
            <Text className={styles.statusTitle}>{irrigating ? '灌溉中' : '待机'}</Text>
            <View className={styles.pressureMetric}>
              <View className={styles.metricLine}>
                <Image className={styles.pressureIcon} src={pressureIcon} mode="aspectFit" />
                <Text className={styles.metricValue}>无</Text>
              </View>
              <Text className={styles.metricLabel}>水压</Text>
            </View>
            <View className={styles.flowMetric}>
              <View className={styles.metricLine}>
                <Image className={styles.flowIcon} src={flowIcon} mode="aspectFit" />
                <Text className={styles.metricValue}>无</Text>
              </View>
              <Text className={styles.metricLabel}>流量</Text>
            </View>
          </View>

          <View className={`${styles.card} ${styles.sensorCard}`}>
            <View className={styles.pumpOne}>
              <Image className={styles.pumpIcon} src={pumpIcon} mode="aspectFit" />
              <Text className={styles.pumpLabel}>水泵1</Text>
            </View>
            <View className={styles.pumpTwo}>
              <Image className={styles.pumpIcon} src={pumpIcon} mode="aspectFit" />
              <Text className={styles.pumpLabel}>水泵2</Text>
            </View>
            <View className={styles.rainStatus} onClick={openRainDialog}>
              <Image className={styles.sensorIcon} src={rainIcon} mode="aspectFit" />
              <Text className={styles.sensorText}>雨感未连接</Text>
            </View>
            <View className={styles.humidityStatus} onClick={openHumidityDialog}>
              <Image className={styles.sensorIcon} src={humidityIcon} mode="aspectFit" />
              <Text className={styles.sensorText}>湿度未连接</Text>
            </View>
          </View>

          <View className={`${styles.card} ${styles.chartCard}`}>
            <View className={styles.donut}>
              <View className={styles.donutHole} />
            </View>
            <View className={styles.legend}>
              <View className={styles.legendRow}>
                <View className={`${styles.legendDot} ${styles.runningDot}`} />
                <Text className={styles.runningText}>灌溉中</Text>
              </View>
              <View className={styles.legendRow}>
                <View className={`${styles.legendDot} ${styles.waitingDot}`} />
                <Text className={styles.waitingText}>等待</Text>
              </View>
              <View className={styles.legendRow}>
                <View className={`${styles.legendDot} ${styles.completedDot}`} />
                <Text className={styles.completedText}>完成</Text>
              </View>
            </View>
            <View
              className={styles.stopButton}
              onClick={() => publishFrame(HOME_STOP_SCHEDULE_FRAME, '停止当前计划', () => appendValveLog('自动灌溉', '关闭'), true, undefined, { kind: 'home', command: 'stop_schedule' })}
            >
              <Image className={styles.controlIcon} src={stopIcon} mode="aspectFit" />
            </View>
            <View
              className={styles.startButton}
              onClick={() => publishFrame(HOME_ADVANCE_GROUP_FRAME, '进入下一组', () => appendValveLog('自动灌溉', '开启'), true, undefined, { kind: 'home', command: 'advance_group' })}
            >
              <Image className={styles.controlIcon} src={startIcon} mode="aspectFit" />
            </View>
          </View>
        </View>}
        {activePage === 'device' && renderDevicePage()}
        {activePage === 'plan' && (showCycleWindow ? renderCyclePage() : renderPlanPage())}
        {activePage === 'test' && renderTestPage()}
        {activePage === 'log' && renderValveLogPage()}

        <View className={styles.bottomNav}>
          <Image className={styles.navItem} src={activePage === 'overview' ? overviewIcon : overviewNormalIcon} mode="aspectFit" onClick={() => setActivePage('overview')} />
          <Image className={styles.navItem} src={activePage === 'device' ? deviceSelectedIcon : deviceIcon} mode="aspectFit" onClick={openWindow2} />
          <Image className={styles.navItem} src={activePage === 'plan' ? planSelectedIcon : planIcon} mode="aspectFit" onClick={() => setActivePage('plan')} />
          <Image className={styles.navItem} src={activePage === 'test' ? testSelectedIcon : testIcon} mode="aspectFit" onClick={() => setActivePage('test')} />
          <Image className={styles.navItem} src={activePage === 'log' ? logNavSelectedIcon : logNavIcon} mode="aspectFit" onClick={() => setActivePage('log')} />
        </View>

        {window2Opening && (
          <View className={styles.window2TransitionOverlay + (window2TransitionClosing ? ' ' + styles.window2TransitionClosing : '') }>
            <View className={styles.window2TransitionCenter}>
              <View className={styles.window2TransitionGlow} />
              <Text className={styles.window2TransitionText}>同步中，请稍候</Text>
            </View>
          </View>
        )}

        {activeDialog === 'rain' && (
          <View className={styles.modalOverlay}>
            <View className={styles.settingDialog}>
              <Image className={styles.dialogBackground} src={rainDialogBackground} mode="scaleToFill" />
              <Text className={styles.dialogTitle}>雨雪延后设置</Text>
              <Image className={styles.rainDialogIcon} src={rainDialogIcon} mode="aspectFit" />
              <View className={styles.rainEditBox}>
                <Image className={styles.editBackground} src={editBackground} mode="scaleToFill" />
                <Input
                  className={styles.dialogInput}
                  type="number"
                  maxLength={2}
                  value={rainInput}
                  onInput={event => setRainInput(String(event.detail.value).replace(/\D/g, ''))}
                />
              </View>
              <Text className={styles.rainUnit}>天</Text>
              <View className={styles.dialogCancelButton} onClick={() => setActiveDialog(null)}>
                <Image className={styles.dialogButtonImage} src={cancelButtonImage} mode="scaleToFill" />
              </View>
              <View className={styles.dialogOkButton} onClick={confirmRainDelay}>
                <Image className={styles.dialogButtonImage} src={okButtonImage} mode="scaleToFill" />
              </View>
            </View>
          </View>
        )}

        {activeDialog === 'humidity' && (
          <View className={styles.modalOverlay}>
            <View className={styles.settingDialog}>
              <Image className={styles.dialogBackground} src={humidityDialogBackground} mode="scaleToFill" />
              <Text className={styles.dialogTitle}>湿度触发设置</Text>
              <Image className={styles.humidityDialogIcon} src={humidityDialogIcon} mode="aspectFit" />
              <View className={styles.humidityEditBox}>
                <Image className={styles.editBackground} src={editBackground} mode="scaleToFill" />
                <Input
                  className={styles.dialogInput}
                  type="number"
                  maxLength={3}
                  value={humidityInput}
                  onInput={event => setHumidityInput(String(event.detail.value).replace(/\D/g, ''))}
                />
              </View>
              <Text className={styles.humidityUnit}>%</Text>
              <View className={styles.dialogCancelButton} onClick={() => setActiveDialog(null)}>
                <Image className={styles.dialogButtonImage} src={cancelButtonImage} mode="scaleToFill" />
              </View>
              <View className={styles.dialogOkButton} onClick={confirmHumidityThreshold}>
                <Image className={styles.dialogButtonImage} src={okButtonImage} mode="scaleToFill" />
              </View>
            </View>
          </View>
        )}


        {showAddDevice && (
          <View className={styles.modalOverlay}>
            <View className={styles.w2AddDialog}>
              <Image className={styles.dialogBackground} src={addDeviceDialogBackground} mode="scaleToFill" />
              <Text className={styles.w2AddTitle}>添加设备</Text>
              <Text className={styles.w2AddTypeLabel}>解码器类型</Text>
              <View className={styles.w2AddTypeButtons}>
                <View className={`${styles.w2AddTypeButton} ${newDeviceType === '传感器' ? styles.w2AddTypeSelected : ''}`} onClick={() => setNewDeviceType('传感器')}>
                  <Image className={styles.w2AddTypeIcon} src={newDeviceType === '传感器' ? sensorTypeSelectedIcon : sensorTypeIcon} mode="aspectFit" />
                </View>
                <View className={`${styles.w2AddTypeButton} ${newDeviceType === '电磁阀' ? styles.w2AddTypeSelected : ''}`} onClick={() => setNewDeviceType('电磁阀')}>
                  <Image className={styles.w2AddTypeIcon} src={newDeviceType === '电磁阀' ? valveTypeSelectedIcon : valveTypeIcon} mode="aspectFit" />
                </View>
              </View>
              <Text className={styles.w2AddAddressLabel}>添加设备地址</Text>
              <View className={styles.w2AddAddressEditor}>
                <View className={`${styles.w2AddAddressStep} ${styles.w2AddAddressPrev}`} onClick={() => setNewDeviceAddress(String(Math.max(20, (parseInt(newDeviceAddress, 10) || 20) - 1)))}><Text>‹</Text></View>
                <View className={styles.w2AddAddressInputFrame}>
                  <Input
                    className={styles.w2AddAddressInput}
                    type="number"
                    value={newDeviceAddress}
                    maxLength={3}
                    onInput={event => {
                      const digits = String(event.detail.value).replace(/\D/g, '').slice(0, 3);
                      setNewDeviceAddress(digits);
                    }}
                    onBlur={() => setNewDeviceAddress(String(clamp(parseInt(newDeviceAddress, 10) || 20, 20, 255)))}
                  />
                </View>
                <View className={`${styles.w2AddAddressStep} ${styles.w2AddAddressNext}`} onClick={() => setNewDeviceAddress(String(Math.min(255, (parseInt(newDeviceAddress, 10) || 20) + 1)))}><Text>›</Text></View>
              </View>
              <View className={styles.w2AddCancel} onClick={() => setShowAddDevice(false)}><Image className={styles.w2AddButtonImage} src={cancelButtonImage} mode="scaleToFill" /></View>
              <View className={styles.w2AddOk} onClick={addDevice}><Image className={styles.w2AddButtonImage} src={okButtonImage} mode="scaleToFill" /></View>
            </View>
          </View>
        )}

        {showW2Editor && (
          <View className={styles.modalOverlay}>
            <View className={styles.w2EditorDialog}>
              <View className={styles.w2EditorRegion1}>
                <Image className={styles.w2EditorRegionBg} src={w2SetRegion1} mode="scaleToFill" />
                <View className={styles.w2EditorAddressGraphic}>
                  <Image className={styles.w2EditorAddressImage} src={w2AddressCombinedImage} mode="scaleToFill" />
                  <View className={styles.w2EditorAddressDigits}>
                    {String(clamp(parseInt(w2EditingAddress, 10) || 1, 1, 255)).padStart(3, '0').split('').map((digit, index) => (
                      <Text key={`address-${index}`} className={styles.w2EditorDigit}>{digit}</Text>
                    ))}
                  </View>
                </View>
                <Image className={styles.w2EditorNameLabel} src={w2DeviceNameLabelImage} mode="scaleToFill" />
                <Text className={styles.w2EditorNameValue} onClick={openW2DeviceRename}>{w2EditingName}</Text>
              </View>
              <View className={styles.w2EditorRegion2}>
                <Image className={styles.w2EditorRegionBg} src={w2SetRegion2} mode="scaleToFill" />
                <View className={styles.w2EditorList}>
                  <Image className={styles.w2EditorListBg} src={w2ChangeIrrListBackground} mode="scaleToFill" />
                  <View className={styles.w2EditorListRows}>
                    {w2GroupNumbers.map(groupNo => {
                      // Keep persisted membership visible; preview only replaces the
                      // stored row when the editor changes its address or type.
                      const editingAddress = clamp(parseInt(w2EditingAddress, 10) || 20, 20, 255);
                      const groupSummary = formatPanelGroupSummary(
                        groupNo,
                        devices,
                        w2EditingIndex,
                        w2GroupPreviewEnabled && groupNo === w2SelectedGroupNo,
                        w2EditingTypeIndex,
                        editingAddress,
                      );
                      return (
                      <View
                        key={`group-${groupNo}`}
                        className={`${styles.w2EditorListRow} ${w2SelectedGroupNo === groupNo ? styles.w2EditorListRowSelected : ''}`}
                        onClick={() => {
                          setW2SelectedGroupNo(groupNo);
                          setW2GroupPreviewEnabled(true);
                          setW2GroupName(`阀组[${groupNo}]`);
                        }}
                      >
                        <Text className={styles.w2EditorListRowText}>
                          {`\u9600\u7ec4[${groupNo}]\u3000${groupSummary}`}
                        </Text>
                      </View>
                      );
                    })}
                  </View>
                </View>
                <View className={styles.w2EditorListButtons}>
                  <View className={styles.w2EditorListButton} onClick={openW2Capacity}><Image className={styles.w2EditorListButtonImage} src={w2OneClickAddImage} mode="scaleToFill" /></View>
                  <View className={styles.w2EditorListButton} onClick={cancelW2Editor}><Image className={styles.w2EditorListButtonImage} src={w2GroupBindCancelImage} mode="scaleToFill" /></View>
                  <View className={styles.w2EditorListButton} onClick={saveW2Editor}><Image className={styles.w2EditorListButtonImage} src={w2SetOkImage} mode="scaleToFill" /></View>
                </View>
              </View>
              <View className={styles.w2EditorRegion3}>
                <Image className={styles.w2EditorRegionBg} src={w2SetRegion3} mode="scaleToFill" />
                <View className={styles.w2EditorTool} onClick={openW2GroupBind}><Image className={styles.w2EditorToolImage} src={w2BindGroupImage} mode="scaleToFill" /></View>
                <View className={styles.w2EditorTool} onClick={openW2ClearTip}><Image className={styles.w2EditorToolImage} src={w2ClearGroupImage} mode="scaleToFill" /></View>
                <View className={styles.w2EditorTool} onClick={() => openW2GroupChoice('delete')}><Image className={styles.w2EditorToolImage} src={w2DeleteGroupImage} mode="scaleToFill" /></View>
                <View className={styles.w2EditorTool} onClick={openW2Rename}><Image className={styles.w2EditorToolImage} src={w2RenameGroupImage} mode="scaleToFill" /></View>
                <View className={styles.w2EditorGroupHeader}>
                  <Image className={styles.w2EditorGroupLabel} src={w2IrrLabelImage} mode="scaleToFill" />
                  <View className={styles.w2EditorGroupDigits}>
                  {String(clamp(w2SelectedGroupNo, 1, 128)).padStart(3, '0').split('').map((digit, index) => (
                    <Text key={`group-value-${index}`} className={`${styles.w2EditorDigit} ${styles.w2EditorGroupDigit}`}>{digit}</Text>
                  ))}
                  </View>
                </View>
              </View>
            </View>
          </View>
        )}

        {showW2Capacity && (
          <View className={styles.modalOverlay}>
            <View className={styles.w2CapacityDialog} onClick={event => event?.stopPropagation?.()}>
              <Text className={styles.w2CapacityTitle}>设置阀组容量</Text>

              <View className={styles.w2CapacitySingleMode} onClick={() => selectW2CapacityMode('single')}>
                <Image
                  className={styles.w2CapacityModeIcon}
                  src={w2CapacityMode === 'single' ? modeSelectOnImage : modeSelectOffImage}
                  mode="scaleToFill"
                />
                <Text className={styles.w2CapacityModeText}>单个阀组</Text>
              </View>
              <View className={styles.w2CapacityAllMode} onClick={() => selectW2CapacityMode('all')}>
                <Image
                  className={styles.w2CapacityModeIcon}
                  src={w2CapacityMode === 'all' ? modeSelectOnImage : modeSelectOffImage}
                  mode="scaleToFill"
                />
                <Text className={styles.w2CapacityModeText}>所有阀组</Text>
              </View>

              <Text className={styles.w2CapacityLabelGroup}>阀组</Text>
              <Text className={`${styles.w2CapacityArrow} ${styles.w2CapacityGroupPrev}`} onClick={() => stepW2CapacityGroup(-1)}>‹</Text>
              <View className={`${styles.w2CapacityValueBox} ${styles.w2CapacityGroupBox}`}>
                <Text className={styles.w2CapacityValueText}>{w2CapacityMode === 'single' ? w2CapacityGroupNo : '-'}</Text>
              </View>
              <Text className={`${styles.w2CapacityArrow} ${styles.w2CapacityGroupNext}`} onClick={() => stepW2CapacityGroup(1)}>›</Text>

              <Text className={styles.w2CapacityLabelValue}>容量</Text>
              <Text className={`${styles.w2CapacityArrow} ${styles.w2CapacityValuePrev}`} onClick={() => stepW2CapacityValue(-1)}>‹</Text>
              <View className={`${styles.w2CapacityValueBox} ${styles.w2CapacityValueBoxBottom}`}>
                <Text className={styles.w2CapacityValueText}>{w2CapacityMode === 'single' ? w2CapacityPending : '-'}</Text>
              </View>
              <Text className={`${styles.w2CapacityArrow} ${styles.w2CapacityValueNext}`} onClick={() => stepW2CapacityValue(1)}>›</Text>

              {[1, 2, 3].map(capacity => (
                <View
                  key={`capacity-${capacity}`}
                  className={`${styles.w2CapacityChoice} ${styles[`w2CapacityChoice${capacity}`]} ${w2CapacityMode === 'all' && w2CapacityPending === capacity ? styles.w2CapacityChoiceSelected : ''}`}
                  onClick={() => w2CapacityMode === 'all' && updateW2CapacityPending(capacity)}
                >
                  <Text className={styles.w2CapacityChoiceText}>{capacity}</Text>
                </View>
              ))}

              <View className={styles.w2CapacityCancel} onClick={cancelW2Capacity}>
                <Image className={styles.w2CapacityButtonImage} src={w2CapacityCancelImage} mode="scaleToFill" />
              </View>
              <View className={styles.w2CapacityOk} onClick={confirmW2Capacity}>
                <Image className={styles.w2CapacityButtonImage} src={w2CapacityOkImage} mode="scaleToFill" />
              </View>
            </View>
          </View>
        )}

        {showW2GroupChoice && (
          <View className={styles.modalOverlay}>
            <View className={styles.w2GroupChoiceDialog} onClick={event => event?.stopPropagation?.()}>
              <Text className={styles.w2GroupChoiceTitle}>
                {w2GroupChoiceMode === 'bind' ? '关联传感器'
                  : w2GroupChoiceMode === 'clear' ? '清空阀组'
                    : w2GroupChoiceMode === 'delete' ? '删除阀组' : '修改名称'} 阀组[{w2GroupChoiceNo}]
              </Text>
              <View className={styles.w2GroupChoiceNumberBox}>
                <Input
                  className={styles.w2GroupChoiceNumberInput}
                  value={String(w2GroupChoiceNo)}
                  type="number"
                  onInput={event => setW2GroupChoiceNo(clamp(parseInt(String(event.detail.value), 10) || 1, 1, 128))}
                />
              </View>
              <View className={`${styles.w2GroupChoiceArrow} ${styles.w2GroupChoicePrev}`} onClick={() => stepW2GroupChoice(-1)}><Text>‹</Text></View>
              <View className={`${styles.w2GroupChoiceArrow} ${styles.w2GroupChoiceNext}`} onClick={() => stepW2GroupChoice(1)}><Text>›</Text></View>
              {(w2GroupChoiceMode === 'bind' || w2GroupChoiceMode === 'clear') && (
                <View className={styles.w2GroupChoiceAll} onClick={confirmW2GroupChoiceAll}>
                  <Image className={styles.w2CapacityButtonImage} src={w2ClearAllImage} mode="scaleToFill" />
                </View>
              )}
              <View className={styles.w2GroupChoiceConfirm} onClick={confirmW2GroupChoiceCurrent}>
                <Image className={styles.w2CapacityButtonImage} src={w2CapacityOkImage} mode="scaleToFill" />
              </View>
              <View className={styles.w2GroupChoiceCancel} onClick={closeW2GroupChoice}>
                <Image className={styles.w2CapacityButtonImage} src={w2GroupBindCancelImage} mode="scaleToFill" />
              </View>
            </View>
          </View>
        )}

        {showW2GroupBind && (
          <View className={styles.modalOverlay}>
            <View className={styles.w2GroupBindDialog}>
              <View className={styles.w2GroupBindRegion1}>
                <Image className={styles.w2GroupBindRegionBg} src={w2GroupBindRegion1} mode="scaleToFill" />
                <View className={`${styles.w2GroupBindField} ${styles.w2GroupBindFieldNumber}`}>
                  <Image className={styles.w2GroupBindLabelBg} src={w2BindGroupNumber} mode="scaleToFill" />
                  <Text className={styles.w2GroupBindValue}>{String(w2SelectedGroupNo)}</Text>
                  <Input className={styles.w2GroupBindInput} value={String(w2SelectedGroupNo)} type="number" onInput={event => setW2SelectedGroupNo(clamp(parseInt(String(event.detail.value), 10) || 1, 1, 128))} />
                </View>
                <View className={`${styles.w2GroupBindField} ${styles.w2GroupBindFieldName}`}>
                  <Image className={styles.w2GroupBindLabelBg} src={w2BindGroupName} mode="scaleToFill" />
                  <Text className={styles.w2GroupBindValue}>{w2GroupName}</Text>
                  <Input className={styles.w2GroupBindInput} value={w2GroupName} onInput={event => setW2GroupName(String(event.detail.value))} />
                </View>
                <View className={`${styles.w2GroupBindField} ${styles.w2GroupBindFieldPump}`}>
                  <Image className={styles.w2GroupBindLabelBg} src={w2BindLinkedPump} mode="scaleToFill" />
                  <Text className={styles.w2GroupBindValue}>{w2SelectedPumpIndexes.map(i => devices[i]?.address).filter(Boolean).join(',')}</Text>
                </View>
                <View className={`${styles.w2GroupBindField} ${styles.w2GroupBindFieldSensor}`}>
                  <Image className={styles.w2GroupBindLabelBg} src={w2BindLinkedSensor} mode="scaleToFill" />
                  <Text className={styles.w2GroupBindValue}>{w2SelectedSensorIndexes.map(i => devices[i]?.address).filter(Boolean).join(',')}</Text>
                </View>
              </View>
              <View className={styles.w2GroupBindRegion2}>
                <Image className={styles.w2GroupBindRegionBg} src={w2GroupBindRegion2} mode="scaleToFill" />
                <Image className={styles.w2GroupBindSelectBg} src={w2BindSelectPump} mode="scaleToFill" />
                <ScrollView className={styles.w2GroupBindList} scrollY>
                  {devices.map((item, index) => item.type === '水泵' ? (
                    <View className={styles.w2GroupBindRow} key={`pump-${item.address}`} onClick={() => toggleGroupSelection(index, true)}>
                      <Text>{item.name}</Text>
                      <Image className={styles.w2GroupBindSelectIcon} src={w2SelectedPumpIndexes.includes(index) ? selectOnImage : selectOffImage} mode="scaleToFill" />
                    </View>
                  ) : null)}
                </ScrollView>
              </View>
              <View className={styles.w2GroupBindRegion3}>
                <Image className={styles.w2GroupBindRegionBg} src={w2GroupBindRegion3} mode="scaleToFill" />
                <Image className={styles.w2GroupBindSelectBg} src={w2BindSelectSensor} mode="scaleToFill" />
                <ScrollView className={styles.w2GroupBindListSensor} scrollY>
                  {devices.map((item, index) => item.type === '传感器' ? (
                    <View className={styles.w2GroupBindRowSensor} key={`sensor-${item.address}`} onClick={() => toggleGroupSelection(index, false)}>
                      <Text>{item.name}</Text>
                      <Image className={styles.w2GroupBindSelectIcon} src={w2SelectedSensorIndexes.includes(index) ? selectOnImage : selectOffImage} mode="scaleToFill" />
                    </View>
                  ) : null)}
                </ScrollView>
              </View>
              <View className={styles.w2GroupBindAction}>
                <Image className={styles.w2GroupBindActionBg} src={w2GroupBindAction} mode="scaleToFill" />
                <View className={styles.w2GroupBindCancel} onClick={() => { setShowW2GroupBind(false); setShowW2Editor(true); }}>
                  <Image className={styles.w2CapacityButtonImage} src={w2GroupBindCancelImage} mode="scaleToFill" />
                </View>
                <View className={styles.w2GroupBindConfirm} onClick={confirmW2GroupBind}>
                  <Image className={styles.w2CapacityButtonImage} src={w2GroupBindConfirm} mode="scaleToFill" />
                </View>
              </View>
            </View>
          </View>
        )}

        {showW2DeviceRename && (
          <View className={styles.modalOverlay}>
            <View className={styles.w2TipDialog}>
              <Image className={styles.dialogBackground} src={tipDialogBackground} mode="scaleToFill" />
              <Text className={styles.w2TipText}>请输入设备名称</Text>
              <Image className={styles.w2RenameInputBg} src={w2GroupRenameEdit} mode="scaleToFill" />
              <View className={styles.w2RenameInputValue}><Text>{w2DeviceNameDraft}</Text></View>
              <Input className={styles.w2RenameInput} value={w2DeviceNameDraft} onInput={event => setW2DeviceNameDraft(String(event.detail.value))} />
              <View className={styles.w2TipCancel} onClick={() => { setShowW2DeviceRename(false); setShowW2Editor(true); }}>
                <Image className={styles.w2CapacityButtonImage} src={cancelButtonImage} mode="scaleToFill" />
              </View>
              <View className={styles.w2TipOk} onClick={() => { setW2EditingName(w2DeviceNameDraft.trim() || w2EditingName); setShowW2DeviceRename(false); setShowW2Editor(true); }}>
                <Image className={styles.w2CapacityButtonImage} src={okButtonImage} mode="scaleToFill" />
              </View>
            </View>
          </View>
        )}

        {showW2Rename && (
          <View className={styles.modalOverlay}>
            <View className={styles.w2TipDialog}>
              <Image className={styles.dialogBackground} src={tipDialogBackground} mode="scaleToFill" />
              <Image className={styles.w2RenameInputBg} src={w2GroupRenameEdit} mode="scaleToFill" />
              <View className={styles.w2RenameInputValue}><Text>{w2GroupName}</Text></View>
              <Input className={styles.w2RenameInput} value={w2GroupName} onInput={event => setW2GroupName(String(event.detail.value))} />
              <View className={styles.w2TipCancel} onClick={() => { setShowW2Rename(false); setShowW2Editor(true); }}>
                <Image className={styles.w2CapacityButtonImage} src={cancelButtonImage} mode="scaleToFill" />
              </View>
              <View className={styles.w2TipOk} onClick={confirmW2Rename}>
                <Image className={styles.w2CapacityButtonImage} src={okButtonImage} mode="scaleToFill" />
              </View>
            </View>
          </View>
        )}

        {showW2ActionTip && (
          <View className={styles.modalOverlay} onClick={() => { if (!syncingDevices) closeW2ActionTip(); }}>
            <View className={styles.w2ActionTipDialog} onClick={(event: any) => event.stopPropagation()}>
              <Image className={styles.dialogBackground} src={actionTipBackground} mode="scaleToFill" />
              <Text className={styles.w2ActionTipText}>
                {syncingDevices
                  ? `同步中，请等待
剩余 ${syncRemaining} 秒`
                  : w2TipText}
              </Text>
            </View>
          </View>
        )}

        {showRoundConfirm && (
          <View className={styles.modalOverlay}>
            <View className={styles.roundConfirmDialog}>
              <Image className={styles.dialogBackground} src={tipDialogBackground} mode="scaleToFill" />
              <Text className={styles.roundConfirmText}>是否开始轮灌？</Text>
              <View className={styles.roundCancel} onClick={() => setShowRoundConfirm(false)}><Image className={styles.w2CapacityButtonImage} src={cancelButtonImage} mode="scaleToFill" /></View>
              <View className={styles.roundOk} onClick={confirmRoundIrrigation}><Image className={styles.w2CapacityButtonImage} src={okButtonImage} mode="scaleToFill" /></View>
            </View>
          </View>
        )}
      </View>
    </View>
  );
}

export default Home;
