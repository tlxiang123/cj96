# CJ96 Current Technical Handoff

Updated: 2026-08-17 09:45 (Asia/Shanghai)

## 1. Current Objective

The current work is reproducing the CJ96 board UI and behavior in the Tuya Ray panel, one board page at a time. The active page is Window2 (device list and device/valve-group editor).

Latest reported issue:

- In the APP Window2 editor, select capacity `2` in the one-click-add popup and press confirm.
- Expected: ungrouped solenoid valves are assigned to valve groups using the same algorithm as the board.
- Previous behavior: the APP popup closed, but the editor still displayed `[empty]` and the board did not execute auto-assignment.

## 2. Important Paths

- Board/FlyThings project: `D:\Install\FlyThingsIDE\bin\workspace\cj96`
- Active Tuya panel project: `C:\Users\Administrator\TuYaMiniProject\miniapp`
- Panel mirror in board repository: `D:\Install\FlyThingsIDE\bin\workspace\cj96\integrations\tuya\panel`
- Tuya bridge in board repository: `D:\Install\FlyThingsIDE\bin\workspace\cj96\integrations\tuya\bridge`
- Historical standalone bridge project: `D:\code\tuya\cj96_tuya_demo`
- Panel output: `C:\Users\Administrator\TuYaMiniProject\miniapp\dist\tuya`
- Board GUI output: `D:\Install\FlyThingsIDE\bin\workspace\cj96\Release\libzkgui.so`
- Embedded bridge runtime: `D:\Install\FlyThingsIDE\bin\workspace\cj96\runtime\bin\cj96_tuya_demo`
- Generated embedded bridge header: `D:\Install\FlyThingsIDE\bin\workspace\cj96\src\generated\TuyaBridgeEmbedded.h`

## 3. Mandatory Working Rules

- Use Python 3 for automation, file processing, builds, and board orchestration.
- Read Context Keeper recovery data before project actions after compaction/resume.
- Do not upload the panel or push GitHub unless the user explicitly requests it.
- The user runs Tuya real-device preview manually.
- The user currently prefers IDE quick-run for board delivery (`Ctrl+Alt+R` / project right-click quick run).
- Do not use `adb reboot`.
- Do not assume the current board binary, MQTT process, panel preview, network, or SD-card state from an older build.
- The worktree is dirty with many existing user changes. Do not revert unrelated files.

## 4. Current Window2 Behavior

### Device list

- Six columns: address, name, type, valve-group number, status, operation.
- The list can scroll.
- Final row contains `Click to add` under address and `Sync` under operation.
- Custom devices from address 20 can show blue text `Delete` in operation.
- APP delete uses command F6 and waits for the full board table to return.

### Add-device dialog

- Device address range is 20 through 255.
- Address 20 is the minimum; decrement must stop at 20.
- Sensor and solenoid-valve types are selectable.

### Device editor

- Displays device address, device name, four valve-group rows, one-click add, cancel, and confirm.
- Right side displays selected valve-group number and association/clear/delete/rename operations.
- Selecting a row must update the selected valve-group display.
- The upper-left back button was removed from APP pages per user request.

### One-click-add popup

- Board-aligned dialog coordinates on the 1024x600 design canvas: `(236, 192, 550, 212)`.
- Capacity choices are 2, 3, and 4; default/selected state is green.
- Confirm must run the board's auto-assignment behavior, not only close the popup.

## 5. Board Auto-Assignment Algorithm

The board local path is in `src/logic/page2Logic.cc`:

1. `closeIrrCapacityWindow(true)` commits the selected capacity.
2. `beginW2AutoAssignPreview()` stores a rollback snapshot.
3. `autoAssignUngroupedValves(capacity)` saves the current edited valve.
4. It collects every ungrouped solenoid valve.
5. Existing valve groups are filled first up to the selected capacity.
6. New groups are created using the smallest unused group number, up to group 128.
7. Device and valve-group lists are refreshed.

Remote APP behavior now uses the same assignment strategy through `autoAssignPage2ValvesFromTuya(capacity)`.

## 6. Latest One-Click-Add Fix

### Panel

File: `integrations/tuya/panel/src/pages/home/index.tsx`

- `confirmW2Capacity()` performs the same local assignment preview so the APP editor updates immediately.
- The editor list now calculates the actual valve addresses for each group and displays:
  - `Valve group[n]  Solenoid valve[addresses]`, or
  - `Valve group[n]  [empty]`.
- Confirm publishes `AA55F7NN55AA`, where `NN` is `02`, `03`, or `04`.
- The panel marks the operation as a pending auto-assignment and waits for the board's full table report.
- The board-returned table replaces APP local data and is the final source of truth.

The repository panel TSX and active Tuya project TSX are synchronized:

- Size: 72571 bytes
- SHA-256: `d8c1721069fef2aa7ec1986e2ef853c19f8ba0b1ed5a37dc3cd5cd1d17eab261`

### Bridge

File: `integrations/tuya/bridge/src/cj96_tuya_demo.c`

- Parses `AA55F7NN55AA` for capacities 2 through 4.
- Writes `group_auto_assign=N` to the GUI home-command channel.

### Board GUI

Files:

- `src/logic/mainLogic.cc`
- `src/logic/page2Logic.cc`

Behavior:

- `mainLogic.cc` recognizes `group_auto_assign=N`.
- Switches the board to Window2.
- Calls `autoAssignPage2ValvesFromTuya(N)`.
- Assigns all existing ungrouped solenoid valves.
- Refreshes Window2 data.
- Queues the complete device table back to the APP.

# 7. Window2 Capacity Dialog Alignment (2026-08-16)

- The active Tuya panel and repository mirror now use the board-sized Window2 capacity dialog.
- The title is 设置阀组容量; mode choices are 单个阀组 and 所有阀组.
- Capacity choices are 1, 2, and 3, using mode_select_off.png and mode_select_on.png at 42 x 42 design pixels.
- Single-group mode shows the current group number and capacity. The left and right arrows change those values, and the right-side capacity frames are not selected.
- All-groups mode temporarily shows - in the two left value boxes and highlights the selected right-side capacity frame.
- Switching modes does not destroy the cached per-group values. Cancel restores the complete dialog snapshot; confirm commits the preview and sends F7.
- Clicking the overlay outside the dialog is treated as cancel.
- The two left value boxes use explicit classes instead of nth-of-type, preventing layout drift when sibling views change.

## 7. CJ96 DP101 Command Frames

Tuya DP:

- DP code: `cj96_raw`
- DP ID: 101

Relevant frames:

- Screen sleep/wake: F0 family.
- Round irrigation on/off: F1 family.
- Stop schedule / advance group: F2 family.
- Rain delay: `AA55F3NN55AA`.
- Humidity threshold: `AA55F4NN55AA`.
- Device synchronization: `AA55F50155AA`.
- Delete custom device: `AA55F6AA55AA`, where `AA` is the address byte 20 through 255.
- Auto-assign valve groups: `AA55F7NN55AA`, where `NN` is 02 through 04.
- Network state reports use the E1 family.

## 8. Device-Table Synchronization

- Protocol prefix: `CJDS1`.
- Supports a single-frame `T` report for small tables.
- Supports chunked `S`, `C`, `E`, and `R` reports with APP `A` acknowledgements.
- Maximum custom address range: 20 through 255 (236 custom devices).
- Default rows: addresses 1, 2, 6, 8, 9, and 10.
- Maximum tested total rows: 242.
- Chunk payload maximum: 900 hex characters.
- Frame maximum: 960 characters.
- Board Window2 discovery countdown: 14,860 ms, rounded up to 15 seconds (236 addresses, 60 ms per address, plus 200 ms start and 500 ms end guards).
- APP cloud/report watchdog: 30 seconds, intentionally separate from the board 15-second countdown; successful transfer ends early.
- Full-capacity protocol test result: 24 chunks, 27 frames, max frame 932, estimated 9.45 seconds.

APP-initiated operations that alter board data must trigger a complete table return. Board-local changes are currently not required to report proactively unless an APP operation requests/causes the change.

## 9. Build Workflow and Verified Artifacts

### Panel build

Run from the active panel project using Python to invoke Node/npm:

```python
import os
import subprocess

env = os.environ.copy()
env["PATH"] = r"D:\Install\NodeJS;" + env.get("PATH", "")
subprocess.run(
    [r"D:\Install\NodeJS\npm.cmd", "run", "build"],
    cwd=r"C:\Users\Administrator\TuYaMiniProject\miniapp",
    env=env,
    check=True,
)
```

Latest result: Ray build passed on 2026-08-16.

### Bridge preparation

```python
import subprocess

subprocess.run(
    ["python", "-X", "utf8", "tools/prepare_tuya_bridge.py"],
    cwd=r"D:\Install\FlyThingsIDE\bin\workspace\cj96",
    check=True,
)
```

This compiles the T113 bridge, copies it to `runtime/bin`, and regenerates `TuyaBridgeEmbedded.h`.

### Board GUI build

```python
import subprocess

subprocess.run(
    ["python", "-X", "utf8", "tools/build_project.py"],
    cwd=r"D:\Install\FlyThingsIDE\bin\workspace\cj96",
    check=True,
)
```

Latest result: bridge compile, GUI compile, and `libzkgui.so` link all passed on 2026-08-14. Existing unused-code warnings remain.

Latest artifact hashes:

- `Release/libzkgui.so`
  - Size: 735012 bytes
  - SHA-256: `6d35990dc83d0f17575ec6bee851574ea9a91ebf748250a366e0c771aa38a7bc`
- `runtime/bin/cj96_tuya_demo`
  - Size: 259220 bytes
  - SHA-256: `7fa3b5b636faf99122784b1ee1df91bae08aceec4874438d7bc85e32e9757e65`

Binary checks passed:

- Bridge contains `AA55F7` and `group_auto_assign=`.
- GUI contains `group_auto_assign=` and the auto-assignment log string.
- `git diff --check` passed for the four main changed source files.

## 10. Current Verification Boundary

Verified now:

- Active panel source matches repository mirror.
- Panel Ray build passes.
- T113 bridge builds and is embedded.
- Board GUI compiles and links.
- F7 command markers exist in final binaries.
- Full-capacity table framing test passes.

Still requiring current-device verification:

- The newly built board GUI/bridge has not been confirmed as the binary currently running on the physical board.
- The latest APP build has not yet been confirmed by a new real-device preview after this fix.
- The full cloud path `APP F7 -> Tuya -> bridge -> GUI -> table report -> APP refresh` still needs one physical test.

## 11. Next Window: First Actions

1. Read this file and the root `AGENTS.md`/project instructions.
2. Do not repeat the completed implementation unless current files differ.
3. Ask the user to run a fresh Tuya real-device preview and IDE quick-run, or execute them only if explicitly requested.
4. Test with at least two ungrouped solenoid valves:
   - Open one valve editor.
   - Press one-click add.
   - Select capacity 2.
   - Confirm.
   - Check that the APP editor immediately shows valve addresses in group rows.
   - Check that the board Window2 shows the same assignments.
   - Check bridge log for `group_auto_assign=2`.
   - Check GUI log for `auto assign capacity=2`.
   - Confirm the APP receives the returned full device table.
5. If APP changes immediately but board does not, inspect F7 cloud/bridge logs before changing UI code.
6. If the board changes but APP reverts to empty, inspect the `CJDS1` returned group bitmap and panel `decodeDeviceTable()`.

## 12. Source-Control State

Relevant changed/untracked paths include:

- Modified: `integrations/tuya/bridge/src/cj96_tuya_demo.c`
- Modified: `integrations/tuya/panel/src/pages/home/index.tsx`
- Modified: `runtime/bin/cj96_tuya_demo`
- Modified: `src/logic/mainLogic.cc`
- Modified: `src/logic/page2Logic.cc`
- Untracked/generated: `src/generated/TuyaBridgeEmbedded.h`

There are many additional pre-existing worktree changes. Preserve them. No GitHub push was performed for this work.

## 13. FlyThingsIDE `Ctrl+Alt+R` 临时快速推送模式

这是当前 CJ96 Window2/Window11 调试时使用的临时运行覆盖，不是永久固件烧录。

### 操作与边界

1. 使用 FlyThingsIDE `Ctrl+Alt+R`，或工程右键中的 quick run。
2. 该模式将当前构建产物推送到板端 `/mnt/extsd`，用于本次调试覆盖。
3. 当前 CJ96 需同步的文件为 `EasyUI.cfg`、`libzkgui.so`、`ui`、`resources`、`font`；以 IDE 传输窗口实际列出的文件为准。
4. 必须等待 IDE 传输窗口完整结束；传输中途不取消、不断电、不在文件尚未完成时判断 UI 逻辑。
5. 重新上电后，板端回到 `/res` 中的内置程序；`/mnt/extsd` 中的快速运行内容不当作永久固件保留。

### 每次快速推送后的验证顺序

1. 先确认工程构建成功。
2. 再确认 IDE 传输窗口已完整结束。
3. 板端进入界面后检查触摸是否响应，并查看 `initLib` 日志。
4. 重点排查 `initLib error`、`symbol not found` 和 Window11 启动后卡死。
5. 只有在构建、传输、板端实际运行三项分别通过后，才开始验证 Window2/Window11 的关联传感器、修改名称、移除和转移操作。

### Window11 布局固定事项

- 当前 Window11 外部点击区域使用 `LayoutPosition(236, 192, 550, 212)`。
- 不要重新引入板端运行库未提供的绝对位置 API；优先沿用已在一键添加弹窗验证过的弹窗处理方式。


## 14. 2026-08-16 APP ??????????

- APP `confirmW2Capacity()` ????????????????????????
- ?????????????????????????
- ????????????????????????????????????? 1..128?
- ???????????????????????? 2??????????????
- APP ????????? `AA55F8GGCC55AA`??????? `AA55F9CC55AA`?? `F7` ??????
- Tuya bridge??? `mainLogic.cc` / `page2Logic.cc` ?????????????????????
- ????APP `npm run build`?bridge `build.py`??? `make -f Release/makefile` ??? 0?
- ????????????? Tuya ?????IDE `Ctrl+Alt+R` ??????????????

## 15. 2026-08-17 APP 一键添加与同步边界

本次 Window2 逻辑固定如下：

- 只有“同步”按钮发送 `DEVICE_SYNC_FRAME`，触发面板重新扫描设备，并显示同步倒计时。
- APP “一键添加”不发送 `DEVICE_SYNC_FRAME`，不重新扫描设备，也不显示设备同步倒计时。
- APP “一键添加”只根据当前 APP 已有设备表生成本地预览，并发送阀组容量配置命令 `F8/F9`。
- 面板完成容量分配后，通过现有配置结果表回传机制返回最终设备表；APP 在收到完整且校验通过的结果表后刷新显示。
- 一键添加时，只对未分组的电磁阀按地址升序分配；已存在阀组的设备保持原阀组，不因再次一键添加而重排。
- 普通同步发现的新设备先显示为未分组，不自动塞入阀组1。
- 一键添加等待结果表期间使用独立的 `deviceReportPending` 状态；超时提示为“一键添加超时，请重新同步”，与设备同步倒计时状态分离。

已验证：

- APP 主工程与仓库镜像 `index.tsx` 内容一致，SHA256 为 `ed4f61b8cfeb9ff5c3237bdb826c78b15c24e36827fee9df9a69751b86fbe790`。
- `C:\\Users\\Administrator\\TuYaMiniProject\\miniapp` 执行 `npm.cmd run build` 返回码为 `0`，Ray 构建完成。
- 尚未执行本轮真实 Tuya 预览、IDE `Ctrl+Alt+R` 快速推送和实体板验证。


## 16. 2026-08-17 APP与本地面板单一数据源方案

当前采用以本地面板为唯一业务数据源：

```text
APP 发送命令
    -> 本地面板读取、修改、保存 DeviceDataStore
    -> 本地面板回传完整设备表
    -> APP 只负责显示
```

- `AA55F50255AA`：仅读取面板当前设备表，不扫描。
- `AA55F50155AA`：只有“同步”按钮触发重新扫描，面板倒计时以板端逻辑为准，APP另用 30 秒作云端回传监听超时。
- “一键添加”只发送 `F8/F9` 容量命令，不再重新扫描，由面板按当前表完成分组后回传最终表。
- 本轮已完成读表、同步、一键添加的面板主数据源改造；关联传感器、清空、改名、转移等编辑操作后续需分别接入面板命令后才能完全改为同一模式，不将 APP 本地预览当作最终数据。

### 流量估算

单条设备记录在当前协议中最大约 60 字节；236 个独立地址的二进制表约 14,160 字节，转十六进制后约 28,320 个 ASCII 字符，加上分片协议头和 MQTT/TLS 开销，一次完整表同步仍是“几十 KB 级”，不会达到 MB 级。按 900 个 hex 字符分片时，大约 32 个数据分片加开始、结束和结果帧。

一个读表请求本身只有 12 个 ASCII 字符左右；高流量操作是反复点开 Window2 并反复请求完整表。版本/hash 查询与缓存已在第17节实现：版本一致时仅回复短确认帧，只有变更时才回传整表。

### 本次构建证据

- Tuya bridge 构建返回 0，产物约 260,256 字节。
- 板端 GUI 执行 `python -X utf8 tools/build_project.py` 返回 0，生成 `Release/libzkgui.so`。
- APP 执行 `npm.cmd run build` 返回 0。
- 尚未执行本轮实机预览、IDE `Ctrl+Alt+R` 快速推送和实体板验证。

## 17. 2026-08-17 APP设备表版本/hash缓存与流量压缩

本轮已完成“版本一致直接显示 APP 旧数据”的实现，缓存按设备 ID 隔离：

- APP第一次没有本地缓存时发送 AA55F50255AA，读取面板当前设备表。
- APP收到完整表并通过 FNV-1a hash 校验后，同时保存设备表、设备表 hash 和保存时间到本地同步存储；缓存 key 为 cj96:device-table:<deviceId>。
- APP再次打开 Window2 时，如果存在有效缓存，发送 AA55F503<8位HEX_HASH>55AA，不再发送完整读表请求。
- 面板返回 CJDS1|V|<session>|S|<count>|<hash> 且 hash、记录数量与缓存一致时，APP直接保留并显示缓存表，结束等待状态，不接收设备表分片。
- 面板返回 CJDS1|V|<session>|C|<count>|<hash> 时，APP继续接收原有 S/C/E/R 完整表分片，校验通过后更新界面、hash 和本地缓存。
- 缓存结构带 schema、deviceId、hash、设备记录和保存时间；缓存损坏、设备 ID 不匹配、hash 不合法或记录结构不完整时自动放弃缓存，回到完整读表路径。
- APP本地编辑阀组、关联、清空或删除阀组时先清理旧缓存 hash，避免面板数据尚未回传时误判为版本一致；最终以面板回传的完整表为准。

### 流量和速度效果

- 236个独立地址的一次完整表仍使用原有可靠分片和整表 hash 校验，正确性路径不变。
- 版本一致时，网络上只发送一个短 hash 查询和一个短版本响应；设备表本身不再重复传输，重复进入 Window2 的设备表流量从约几十 KB 降到几十字节级，等待时间也从完整分片等待缩短为一次短响应。
- 版本发生变化时仍然完整读取新表，不用旧缓存冒充新数据；因此压缩只发生在确认一致的场景。
- 这里使用 hash 作为版本指纹，不依赖本地递增版本号；面板任何设备地址、类型、名称、连接状态或阀组位图变化都会导致 hash 变化。

### 本轮构建证据

- C:\\Users\\Administrator\\TuYaMiniProject\\miniapp 执行 npm.cmd run build 返回码 0，Ray 构建完成。
- APP主工程与仓库镜像的 Window2 TSX 已同步。
- python -X utf8 tools/test_tuya_device_sync_protocol.py 回归测试通过：rows=242、custom=236、chunks=24、hash=87FA17F4。
- python -X utf8 tools/build_project.py 返回码 0，板端 GUI 与 Tuya bridge 构建完成。
- 尚未执行 Tuya 真机预览、IDE Ctrl+Alt+R 快速推送和实体板验证。

## 18. 2026-08-17 Window2 refresh latency optimization

Observed path before this change: APP showed the cloud publish callback as send succeeded while the actual board table was still pending. The board consumed the Window2 Tuya command only from the 1000 ms UI timer, and the bridge used a 100 ms main-loop sleep plus a 350 ms device-report frame interval.

Changes:
- APP requestCurrentDeviceTable no longer shows a misleading success toast for the version/read query. The cached Window2 table remains visible immediately; the query runs silently in the background.
- Board handleTuyaDeviceSyncCommand now runs on the existing 100 ms timer instead of the 1000 ms timer. Normal home commands remain on the 1000 ms timer.
- Tuya bridge device-report interval changed from 350 ms to 80 ms. Bridge loop sleep changed from 100 ms to 50 ms. Protocol frames, hash validation, ACK handling, and the full-sync path are unchanged.

Build verification: APP npm.cmd run build returned 0; board python -X utf8 tools/build_project.py returned 0. Runtime delivery and real APP/board timing still require the user IDE Ctrl+Alt+R quick-run and a real Window2 test.


## 19. 2026-08-17 Window2 group-capacity sync

Implemented the confirmed design: the board remains the single source of truth, and entering APP Window2 performs a cached display plus a background version/hash check. The board now returns the complete valve-group maximum-capacity table together with a changed device table.

- Added protocol frame CJDS1|G|<SESSION>|<256-hex-capacity-bytes>|<8-hex-joint-hash> before T or S/C/E/R full-table frames. The 128 bytes map to valve groups 1..128; each value is clamped to 1..3, including empty groups.
- The version hash is now FNV-1a over device-table-bytes + 128 capacity bytes, so changing only a group capacity invalidates the APP cache.
- APP cache schema is now 2 and stores groupCapacities for all 128 groups. Schema-1 caches are discarded and refreshed from the board.
- APP validates the G frame, keeps the capacity map during both single-frame and chunked table transfers, updates the Window2 capacity state, and writes devices plus capacities together to cache.
- Active APP project and repository panel mirror are byte-identical.
- Protocol regression passed: 
ows=242, custom=236, chunks=24, rames=28, max_frame=932, joint hash 35F7AEF4.
- APP build passed with 
pm.cmd run build; board GUI and Tuya bridge build passed with python -X utf8 tools/build_project.py.
- Built artifacts: Release/libzkgui.so SHA-256 5b68192168e2b424784f3f04b2b3aea4f003cd913d663e615e6f42d0f18df04d; 
untime/bin/cj96_tuya_demo SHA-256 31f3ea783b6a299c1365c26773c7e5baf2b1bed8cf06df51d129a113b7e47a1a.
- Not uploaded, not pushed, and not flashed. Real-device verification remains: user runs IDE Ctrl+Alt+R, opens APP Window2, checks that empty-group capacities and capacity-only changes are reflected without a new address scan.
