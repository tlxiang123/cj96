from __future__ import annotations

import argparse
import json
import logging
import re
import sys
import threading
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any


LOG = logging.getLogger("wechat-order-bot")


@dataclass(frozen=True)
class Rule:
    pattern: re.Pattern[str]
    reply: str


def load_config(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as file:
        config = json.load(file)

    groups = config.get("groups")
    if not isinstance(groups, list) or not groups or not all(isinstance(x, str) and x for x in groups):
        raise ValueError("config.json 中 groups 必须是非空的群名字符串数组")
    return config


def compile_rules(config: dict[str, Any]) -> list[Rule]:
    rules: list[Rule] = []

    for phrase, reply in config.get("fixed_rules", {}).items():
        # 固定规则按“包含该短语”匹配，群消息前后可以有其他说明文字。
        rules.append(Rule(re.compile(re.escape(str(phrase)), re.IGNORECASE), str(reply)))

    for item in config.get("regex_rules", []):
        pattern = item.get("pattern")
        reply = item.get("reply")
        if not pattern or reply is None:
            raise ValueError("regex_rules 中每一项都必须包含 pattern 和 reply")
        rules.append(Rule(re.compile(pattern, re.IGNORECASE), str(reply)))

    if not rules:
        raise ValueError("至少需要配置一条 fixed_rules 或 regex_rules")
    return rules


def find_reply(content: str, rules: list[Rule]) -> str | None:
    normalized = content.strip()
    for rule in rules:
        match = rule.pattern.search(normalized)
        if match:
            try:
                return match.expand(rule.reply)
            except re.error as exc:
                raise ValueError(f"回复模板无效: {rule.reply!r}") from exc
    return None


class OrderBot:
    def __init__(self, config: dict[str, Any], dry_run: bool = False) -> None:
        self.groups: list[str] = config["groups"]
        self.rules = compile_rules(config)
        self.delay = max(0.0, float(config.get("reply_delay_seconds", 0.0)))
        self.cooldown = max(0.0, float(config.get("dedup_seconds", 10.0)))
        self.dry_run = dry_run
        self._recent: dict[tuple[str, str, str], float] = {}
        self._lock = threading.Lock()

    def _is_duplicate(self, group: str, sender: str, content: str) -> bool:
        now = time.monotonic()
        key = (group, sender, content)
        with self._lock:
            expired = [item for item, seen_at in self._recent.items() if now - seen_at > self.cooldown]
            for item in expired:
                self._recent.pop(item, None)
            if key in self._recent:
                return True
            self._recent[key] = now
            return False

    def on_message(self, msg: Any, chat: Any) -> None:
        # wxauto4 的 SelfMessage 是自己发出的消息；只处理 FriendMessage。
        try:
            from wxauto4.msgs import FriendMessage
        except ImportError:
            FriendMessage = None  # type: ignore[assignment,misc]

        if FriendMessage is not None and not isinstance(msg, FriendMessage):
            return

        content = getattr(msg, "content", "")
        if not isinstance(content, str) or not content.strip():
            return

        group = str(getattr(chat, "who", chat))
        sender = str(getattr(msg, "sender", "未知发送者"))
        reply = find_reply(content, self.rules)
        if reply is None or self._is_duplicate(group, sender, content):
            return

        LOG.info("命中消息 group=%s sender=%s content=%r reply=%r", group, sender, content, reply)
        if self.dry_run:
            LOG.info("演练模式：未发送消息")
            return
        if self.delay:
            time.sleep(self.delay)
        chat.SendMsg(reply)

    def run(self) -> None:
        try:
            from wxauto4 import WeChat
        except ImportError as exc:
            raise RuntimeError("未安装 wxauto4，请先运行: pip install wxauto4") from exc

        wx = WeChat()
        for group in self.groups:
            LOG.info("开始监听群聊: %s", group)
            wx.AddListenChat(group, self.on_message)

        LOG.info("机器人已启动，按 Ctrl+C 停止")
        try:
            wx.KeepRunning()
        except KeyboardInterrupt:
            LOG.info("正在停止")
        finally:
            wx.StopListening()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="监听微信群抢单消息并自动回复")
    parser.add_argument("--config", type=Path, default=Path(__file__).with_name("config.json"))
    parser.add_argument("--dry-run", action="store_true", help="只打印命中结果，不发送微信消息")
    parser.add_argument("--test", metavar="TEXT", help="仅测试一条文本的匹配结果，不连接微信")
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(levelname)s %(message)s")
    try:
        config = load_config(args.config)
        bot = OrderBot(config, dry_run=args.dry_run)
        if args.test is not None:
            reply = find_reply(args.test, bot.rules)
            print(reply if reply is not None else "未匹配")
            return 0
        bot.run()
        return 0
    except (OSError, ValueError, RuntimeError, json.JSONDecodeError) as exc:
        LOG.error("%s", exc)
        return 1


if __name__ == "__main__":
    sys.exit(main())
