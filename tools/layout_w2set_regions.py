#!/usr/bin/env python3
"""Rebuild w2set_window as three nested Window1-style regions."""

from pathlib import Path

from ftu_style import decode_ftu, encode_ftu


ROOT = Path(__file__).resolve().parents[1]
FTU_PATH = ROOT / "ui" / "main.ftu"

REGIONS = (
    ("W2SetRegion1Window", 110040, {"left": 12, "top": 8, "width": 980, "height": 88}),
    ("W2SetRegion2Window", 110041, {"left": 12, "top": 102, "width": 590, "height": 232}),
    ("W2SetRegion3Window", 110042, {"left": 612, "top": 102, "width": 383, "height": 232}),
)


def find_control(node: object, caption: str) -> dict:
    if isinstance(node, dict):
        if node.get("caption") == caption:
            return node
        for value in node.values():
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    elif isinstance(node, list):
        for value in node:
            try:
                return find_control(value, caption)
            except KeyError:
                pass
    raise KeyError(caption)


def remove_child(parent: dict, caption: str) -> tuple[str, dict]:
    for key, value in list(parent.items()):
        if isinstance(value, dict) and value.get("caption") == caption:
            del parent[key]
            return key, value
    raise KeyError(caption)


def set_pos(control: dict, left: int, top: int, width: int | None = None,
            height: int | None = None) -> None:
    position = dict(control.get("position", {}))
    position.update(left=left, top=top)
    if width is not None:
        position["width"] = width
    if height is not None:
        position["height"] = height
    control["position"] = position


def make_region(caption: str, control_id: int, position: dict) -> dict:
    return {
        "backgroundPic": "window_info_panel.png",
        "beepEnable": True,
        "caption": caption,
        "id": control_id,
        "visible": True,
        "position": {"height": position["height"], "left": position["left"],
                      "top": position["top"], "width": position["width"]},
    }


def patch() -> None:
    data, header, _ = decode_ftu(FTU_PATH)
    w2set = find_control(data, "w2set_window")

    # Remove prior generated regions so this script is idempotent.
    generated = {caption for caption, _, _ in REGIONS}
    for key, value in list(w2set.items()):
        if isinstance(value, dict) and value.get("caption") in generated:
            del w2set[key]

    # Move existing controls into their requested region. The address edit box
    # remains hidden because TextView1 is the dynamic address image surface.
    controls: dict[str, tuple[str, dict]] = {}
    for caption in (
        "W2_AddressEditText", "TextView1", "TextView3", "W2_NameEditText",
        "ChangeIrr_ListView", "IrrNum_TextView", "ClearIrr_Button",
        "GroupBind_Button", "W2_DelButton",
    ):
        key, control = remove_child(w2set, caption)
        controls[caption] = (key, control)

    # GroupNameEditText currently lives in GroupBindValueWindow. It belongs in
    # the new w2set region, so move that existing control rather than cloning
    # its ID and creating ambiguous findControlByID results.
    group_name_parent = find_control(data, "GroupBindValueWindow")
    group_name_key, group_name = remove_child(group_name_parent, "GroupNameEditText")
    controls["GroupNameEditText"] = (group_name_key, group_name)

    # Region 1: address and device name.
    r1 = make_region(*REGIONS[0])
    address = controls["W2_AddressEditText"][1]
    address["visible"] = False
    set_pos(address, 30, 18)
    text1 = controls["TextView1"][1]
    set_pos(text1, 28, 6, 199, 77)
    text3 = controls["TextView3"][1]
    set_pos(text3, 470, 20, 97, 48)
    name = controls["W2_NameEditText"][1]
    set_pos(name, 605, 22, 144, 41)
    for caption in ("W2_AddressEditText", "TextView1", "TextView3", "W2_NameEditText"):
        key, control = controls[caption]
        r1[key] = control

    # Region 2: the irrigation array list. Only IrrArr_SubItem is retained.
    r2 = make_region(*REGIONS[1])
    list_view = controls["ChangeIrr_ListView"][1]
    set_pos(list_view, 22, 30, 546, 184)
    item = list_view.get("item", {})
    item["position"] = {"height": 37, "left": 0, "top": 0, "width": 541}
    item["subItem"] = [sub for sub in item.get("subItem", [])
                        if sub.get("caption") == "IrrArr_SubItem"]
    if len(item["subItem"]) != 1:
        raise RuntimeError("IrrArr_SubItem was not found")
    set_pos(item["subItem"][0], 5, 0, 531, 38)
    r2[controls["ChangeIrr_ListView"][0]] = list_view

    # Region 3: group value and group operations.
    r3 = make_region(*REGIONS[2])
    irr_num = controls["IrrNum_TextView"][1]
    set_pos(irr_num, 22, 14, 126, 38)
    clear = controls["ClearIrr_Button"][1]
    set_pos(clear, 22, 58, 110, 113)
    bind = controls["GroupBind_Button"][1]
    set_pos(bind, 154, 58, 206, 38)
    delete = controls["W2_DelButton"][1]
    set_pos(delete, 154, 111, 130, 40)
    group_name = controls["GroupNameEditText"][1]
    set_pos(group_name, 154, 170, 144, 41)
    for caption in ("IrrNum_TextView", "ClearIrr_Button", "GroupBind_Button",
                    "W2_DelButton", "GroupNameEditText"):
        key, control = controls[caption]
        r3[key] = control

    # Keep the original action buttons at the bottom of w2set_window. Insert
    # the three nested windows before them so they render as the background
    # containers for their children.
    rebuilt: dict = {}
    rebuilt["window__w2_region1"] = r1
    rebuilt["window__w2_region2"] = r2
    rebuilt["window__w2_region3"] = r3
    rebuilt.update(w2set)
    w2set.clear()
    w2set.update(rebuilt)
    FTU_PATH.write_bytes(encode_ftu(data, header))


def verify() -> None:
    data, _, _ = decode_ftu(FTU_PATH)
    w2set = find_control(data, "w2set_window")
    for caption, control_id, _ in REGIONS:
        region = find_control(w2set, caption)
        if region.get("id") != control_id or region.get("backgroundPic") != "window_info_panel.png":
            raise RuntimeError(f"invalid region {caption}")
    r1 = find_control(w2set, "W2SetRegion1Window")
    for caption in ("TextView1", "TextView3", "W2_NameEditText"):
        find_control(r1, caption)
    r2 = find_control(w2set, "W2SetRegion2Window")
    list_view = find_control(r2, "ChangeIrr_ListView")
    subitems = list_view["item"].get("subItem", [])
    if [item.get("caption") for item in subitems] != ["IrrArr_SubItem"]:
        raise RuntimeError("ChangeIrr_ListView still contains an extra subitem")
    r3 = find_control(w2set, "W2SetRegion3Window")
    for caption in ("IrrNum_TextView", "ClearIrr_Button", "GroupBind_Button",
                    "W2_DelButton", "GroupNameEditText"):
        find_control(r3, caption)


if __name__ == "__main__":
    patch()
    verify()
    print("w2set_window rebuilt with three nested regions")
