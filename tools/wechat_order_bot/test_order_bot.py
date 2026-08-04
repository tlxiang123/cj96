import unittest

from order_bot import compile_rules, find_reply


class RuleTests(unittest.TestCase):
    def setUp(self) -> None:
        self.rules = compile_rules(
            {
                "fixed_rules": {"xxxx": "7"},
                "regex_rules": [{"pattern": r"扣\s*([0-9]+)", "reply": r"\1"}],
            }
        )

    def test_fixed_phrase(self) -> None:
        self.assertEqual(find_reply("有新单 xxxx 马上开始", self.rules), "7")

    def test_extract_number(self) -> None:
        self.assertEqual(find_reply("订单来了，扣 6", self.rules), "6")

    def test_no_match(self) -> None:
        self.assertIsNone(find_reply("普通聊天消息", self.rules))


if __name__ == "__main__":
    unittest.main()
