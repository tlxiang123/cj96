1: 不要使用powershell 用python
2 # Third-party UI assets

- `irrigation_field.jpg`: photo by Elibet Valencia Munoz on Unsplash,
  <https://unsplash.com/photos/z4_bNziWTWE>, used under the Unsplash License.
- `centrifugal_pump.jpg`: "Water Pump" by Visthetique on Sketchfab,
  <https://sketchfab.com/3d-models/8216938e5fb54e5db4879949dd6de124>,
  used under CC BY 4.0.
- UI icons: Lucide Icons 1.14.0, <https://github.com/lucide-icons/lucide>,
  used under the ISC License.
3 make的目录是D:\Install\FlyThingsIDE\sdk\toolchains\t113\bin 每次修改完了之后make all
4 这里是说明文档 C:\Users\Administrator\Documents\Codex\2026-06-23\d\outputs\flythings_developer_docs_local.md
5 本地的所有示例在D:\Install\FlyThingsIDE\bin\workspace\basedemo-new_t113_1024_600

## FTU 可编辑布局规则

1. 每次修改 UI 后，用户必须能够在 FlyThingsIDE 的 `.ftu` 文件中自行选中、拖动和调整相关控件。
2. 图标、文字、按钮、输入框和背景框应保持为 `.ftu` 中的独立控件；不要把需要单独调整的图标或文字合并到整页背景图片中。
3. 背景 PNG 只用于边框、底色等不需要单独排版的装饰。需要移动的文字必须使用 TextView、Button 文本或其他可编辑文字控件。
4. 新增或修改控件后，应保留清晰的 caption 和唯一 ID，确保用户能在 FlyThingsIDE 控件树中找到它。
5. 不要覆盖用户已经在 `.ftu` 中完成的位置和尺寸；除非用户明确要求重新排版，否则只修改本次需求涉及的属性。
6. 使用 Python 处理 `.ftu` 和图片资源，不使用 PowerShell 修改文件内容。
7. 修改完成后执行 `make all`，并确认生成代码可以正常编译和链接。
8. `make all` 只更新 `Release/libzkgui.so`，不会自动更新板端 UI。凡是修改 `.ftu` 或图片资源，部署/烧录时必须同时更新当前 `ui/*.ftu`、所引用的 `resources/*` 和 `Release/libzkgui.so`，确保 IDE 布局与板端 UI 使用同一版本。
9. 使用 Python 在 FlyThingsIDE 外部修改 `.ftu` 后，必须在 IDE 中关闭并重新打开该页面，或执行重新载入；不要继续编辑 IDE 内存中尚未刷新的旧布局，否则保存时会覆盖新 `.ftu`。
10. 不在 C++ 逻辑中使用 `setPosition`、`setSize` 等方式改变控件布局。板端控件位置和尺寸必须来自 `.ftu`，保证 FlyThingsIDE 中显示的位置与实际运行一致。
11. 新增或修改 FTU 控件时，控件键名必须使用 FlyThingsIDE 原生数字形式（例如 `textview__266`、`button__239`），不要使用自定义字符串键名；新增 ID 必须先全局检查，不能与任何已有控件 ID 重复。否则可能出现板端能显示，但 FlyThingsIDE 控件树/画布不显示或无法编辑的问题。

## 调试部署注意

- 不执行 `adb reboot`：这个板子没有 SD 挂载时 `/mnt/extsd` 是重启后会丢的目录；重启必然回到 `/res` 老界面。
- 调试方式：推 `/mnt/extsd/EasyUI.cfg` + `lib` + `ui` + `resources` + `font`，然后只重启 `zkgui` 服务。
- 烧录/部署前后必须确认电脑 IDE 工程文件、Release 副本和板端文件同步一致；例如修改主界面时至少核对 `ui/main.ftu`、`Release/remote_main.ftu`、`/mnt/extsd/ui/main.ftu` 的 MD5 一致，不能只以编译成功或板端显示为准。

## 联网功能规则

- 以后所有 cj96 联网相关功能都按“有线优先”处理：如果 Wi-Fi 和有线同时存在，实际联网请求优先走 `eth0`，不要被 Wi-Fi 默认路由影响。
- Wi-Fi 手动连接或系统自动连接成功后，都必须检测该 SSID 是否能连接外网。检测时要强制走 `wlan0`，不能让有线网络代替 Wi-Fi 通过检测。
- 如果某个 SSID 连上后不能访问外网，应主动断开该 Wi-Fi，并弹窗提示用户，第一行显示实际 SSID，第二行显示“无法连接外网”。
- Wi-Fi 联网验证交互按手机类似流程处理：连接完成后先显示“正在检查网络”，不要立刻显示“已连接”；验证成功后显示绿色“已连接”，保持与开阀/关阀成功提示相同的短时间后自动消失；验证失败应在开始检测后约 5-8 秒内给出结果，失败弹窗保持显示，直到用户触摸关闭。
- Wi-Fi 验证失败后不要让系统下次继续自动连接这个坏网：优先删除 supplicant 保存的 network 配置并保存；如果当前拿不到 network id，则至少把 SSID 记录到 `/mnt/extsd/cj96_wifi_bad_ssid.txt`，后续自动连上该 SSID 时立即断开。用户手动重新连接该 SSID 时，允许重新验证，成功后清除坏网记录。
- NTP 同步成功后要记录本次成功的服务器字符串到 `/mnt/extsd/cj96_ntp_last_server.txt`；下次同步时优先尝试这个服务器，再按默认列表轮询。
