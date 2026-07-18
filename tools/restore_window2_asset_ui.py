#!/usr/bin/env python3
"""Restore Window2's image-backed address and delete-group controls."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu
from layout_window3_customer import find_control


ROOT = Path(__file__).resolve().parents[1]
FTU = ROOT / "ui" / "main.ftu"


def main() -> None:
    data, header, _ = decode_ftu(FTU)
    window = find_control(data, "w2set_window")

    address_edit = find_control(window, "W2_AddressEditText")
    address_edit["visible"] = False

    address = find_control(window, "TextView1")
    address["text"] = ""
    address["backgroundPic"] = "w2_set_address_combined_204.png"
    address["touchable"] = False
    address["position"] = {"height": 77, "left": 338, "top": 0, "width": 199}

    delete_group = find_control(window, "ClearIrr_Button")
    delete_group.pop("text", None)
    delete_group["picTab"] = {
        "pic0": "w2_set_delete_group_110x113_v2.png",
        "pic2": "w2_set_delete_group_110x113_v2.png",
    }
    delete_group["iconPosition"] = {"height": 113, "left": 0, "top": 0, "width": 110}
    delete_group["position"] = {"height": 113, "left": 65, "top": 46, "width": 110}

    FTU.write_bytes(encode_ftu(data, header))
    print("restored Window2 image-backed address and delete-group controls")


if __name__ == "__main__":
    main()
