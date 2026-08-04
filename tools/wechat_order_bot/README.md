# 微信群抢单自动回复

这个脚本监听指定微信群，发现符合规则的新消息后立即回复对应数字。仅支持 Windows 微信电脑版 4.x。

## 安装

建议使用 Python 3.10-3.12。先登录并保持微信电脑版正常运行，然后在 PowerShell 中执行：

```powershell
cd D:\Install\FlyThingsIDE\bin\workspace\cj96\tools\wechat_order_bot
py -m venv .venv
.\.venv\Scripts\Activate.ps1
python -m pip install -r requirements.txt
Copy-Item config.example.json config.json
```

编辑 `config.json`，把 `groups` 改成群聊的完整名称，并按需修改规则。

- `fixed_rules`：消息中包含左侧固定短语时，回复右侧内容。
- `regex_rules`：正则匹配；示例会将“扣7”“扣 6”分别回复为“7”“6”。
- `reply_delay_seconds`：命中后等待多少秒发送；抢单场景可保持 `0.0`。
- `dedup_seconds`：同一群、同一发送者、同一内容的去重时间。

先测试规则，不会连接微信：

```powershell
python order_bot.py --test "新单来了，扣7"
```

建议先用演练模式观察日志，此模式监听微信但不会发送：

```powershell
python order_bot.py --dry-run
```

确认无误后正式运行：

```powershell
python order_bot.py
```

按 `Ctrl+C` 停止。运行期间不要退出微信、锁定 Windows 会话或关闭监听群的独立聊天窗口。UI 自动化可能因微信升级、弹窗或窗口状态失效；请先在测试群验证。高频自动化可能触发微信风控，请控制使用频率并遵守群规则及微信使用条款。
