#!/usr/bin/env python3
"""Compile the Reborn Mahjong action medallions into the legacy button atlases."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter


KEY_COLOR = (255, 210, 210, 255)

# Source rectangles in the approved 2x2 concept sheet.
SOURCE_CROPS = {
    "logs": (81, 82, 589, 585),
    "maps": (665, 82, 1172, 585),
    "help": (81, 668, 589, 1171),
    "system": (664, 668, 1172, 1171),
}

# Legacy runtime rectangles.  The first state is 51x51; hover is 46x46.
DESTINATIONS = {
    "logs": ((0, 173, 51, 51), (209, 179, 46, 46)),
    "system": ((51, 173, 51, 51), (260, 179, 46, 46)),
    "help": ((102, 173, 51, 51), (310, 179, 46, 46)),
    "maps": ((153, 173, 51, 51), (361, 179, 46, 46)),
}

# Large textual controls used by the adventure HUD and modal dialogs.  Labels
# are lifted from the localized source atlas, so the generated art remains
# language-complete without depending on a host-installed font.
LARGE_BUTTON_PAIRS = (
    (0, 0),       # done
    (172, 0),     # ok
    (344, 0),     # dismiss
    (516, 0),     # undo
    (688, 0),     # close
    (688, 45),    # cancel
    (0, 92),      # info
    (344, 92),    # menu
)


def label_mask(button: Image.Image) -> Image.Image:
    """Extract the red normal-state lettering while rejecting its old frame."""
    rgb = button.convert("RGB")
    mask = Image.new("L", button.size, 0)
    pixels = mask.load()
    for y in range(5, button.height - 5):
        for x in range(5, button.width - 5):
            red, green, blue = rgb.getpixel((x, y))
            prominence = red - max(green, blue)
            if red > 78 and prominence > 42:
                pixels[x, y] = min(255, prominence * 4)
    return mask


def metal_button(size: tuple[int, int], mask: Image.Image, hover: bool) -> Image.Image:
    width, height = size
    result = Image.new("RGBA", size, KEY_COLOR)
    draw = ImageDraw.Draw(result, "RGBA")
    fill = (25, 34, 35, 255) if not hover else (42, 49, 46, 255)
    gold = (139, 105, 54, 255) if not hover else (211, 164, 78, 255)
    draw.rectangle((0, 0, width - 1, height - 1), fill=(8, 11, 12, 255))
    draw.rectangle((2, 2, width - 3, height - 3), fill=fill, outline=gold, width=2)
    draw.rectangle((5, 5, width - 6, height - 6), outline=(74, 92, 91, 255), width=1)
    draw.line((7, 7, width - 8, 7), fill=(145, 139, 104, 180), width=1)
    draw.line((7, height - 8, width - 8, height - 8), fill=(6, 9, 10, 230), width=1)

    shadow = mask.filter(ImageFilter.GaussianBlur(1.0))
    result.paste((0, 0, 0, 190), (1, 2), shadow)
    ink = (231, 207, 143, 255) if not hover else (255, 232, 157, 255)
    result.paste(ink, (0, 0), mask)
    return result


def restyle_large_buttons(atlas: Image.Image) -> None:
    for x, y in LARGE_BUTTON_PAIRS:
        normal = atlas.crop((x, y, x + 86, y + 46))
        mask = label_mask(normal)
        atlas.paste(metal_button((86, 46), mask, False), (x, y))
        atlas.paste(metal_button((86, 46), mask, True), (x + 86, y))


def compile_order_atlas(base_path: Path, output_path: Path) -> None:
    atlas = Image.open(base_path).convert("RGBA")
    mask = label_mask(atlas.crop((0, 0, 86, 46)))
    atlas.paste(metal_button((86, 46), mask, False), (0, 0))
    atlas.paste(metal_button((86, 46), mask, True), (86, 0))
    atlas.convert("RGB").save(output_path)


def render_icon(source: Image.Image, crop: tuple[int, int, int, int], size: int,
                hover: bool) -> Image.Image:
    icon = source.crop(crop).resize((size, size), Image.Resampling.LANCZOS)
    if hover:
        rgb = ImageEnhance.Brightness(icon.convert("RGB")).enhance(1.16)
        icon = Image.merge("RGBA", (*rgb.split(), icon.getchannel("A")))
    return icon


def compile_atlas(base_path: Path, source: Image.Image, output_path: Path) -> None:
    atlas = Image.open(base_path).convert("RGBA")
    restyle_large_buttons(atlas)
    for name, destinations in DESTINATIONS.items():
        for hover, (x, y, width, height) in enumerate(destinations):
            atlas.paste(KEY_COLOR, (x, y, x + width, y + height))
            icon = render_icon(source, SOURCE_CROPS[name], min(width, height), bool(hover))
            atlas.alpha_composite(icon, (x, y))

    # The engine uses this historical pink color as the sprite-sheet key.
    flattened = Image.new("RGBA", atlas.size, KEY_COLOR)
    flattened.alpha_composite(atlas)
    flattened.convert("RGB").save(output_path)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--base", type=Path, required=True)
    parser.add_argument("--base-ru", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--output-ru", type=Path, required=True)
    parser.add_argument("--order-base", type=Path)
    parser.add_argument("--order-base-ru", type=Path)
    parser.add_argument("--order-output", type=Path)
    parser.add_argument("--order-output-ru", type=Path)
    args = parser.parse_args()

    source = Image.open(args.source).convert("RGBA")
    compile_atlas(args.base, source, args.output)
    compile_atlas(args.base_ru, source, args.output_ru)
    order_args = (args.order_base, args.order_base_ru,
                  args.order_output, args.order_output_ru)
    if any(order_args):
        if not all(order_args):
            parser.error("all four order button paths are required together")
        compile_order_atlas(args.order_base, args.order_output)
        compile_order_atlas(args.order_base_ru, args.order_output_ru)
    print(f"Built {args.output} and {args.output_ru}")


if __name__ == "__main__":
    main()
