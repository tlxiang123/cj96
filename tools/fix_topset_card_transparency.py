#!/usr/bin/env python3
"""Regenerate only the six TopSet cards with transparent outer corners."""

from rebuild_page1topset_cards import CARD_SPECS, RESOURCE_DIR, build_card


def main() -> None:
    for _, (source_name, crop, output_name) in CARD_SPECS.items():
        build_card(source_name, crop).save(
            RESOURCE_DIR / output_name, optimize=True
        )
    print("rebuilt six TopSet cards with transparent rounded corners")


if __name__ == "__main__":
    main()
