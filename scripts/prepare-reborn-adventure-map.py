#!/usr/bin/env python3
"""Build the Reborn adventure screen from independent map artwork.

The generated terrain is deliberately kept free of gameplay markings.  This
script restores province borders from the authoritative land polygons and
builds a Reborn-specific right-side interface panel. Keeping the terrain,
panel and gameplay sprites separate lets them migrate independently.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter


MAP_WIDTH = 776
SCREEN_WIDTH = 1024
SCREEN_HEIGHT = 768
ANTIALIAS = 4
TERRAIN_FIT_WIDTH = 799
TERRAIN_FIT_HEIGHT = 789
TERRAIN_CROP_X = 17
TERRAIN_CROP_Y = 5


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--terrain", type=Path, required=True)
    parser.add_argument("--panel-texture", type=Path, required=True)
    parser.add_argument("--lands", type=Path, required=True)
    parser.add_argument("--terrain-out", type=Path, required=True)
    parser.add_argument("--panel-out", type=Path)
    parser.add_argument("--screen-out", type=Path, required=True)
    return parser.parse_args()


def draw_borders(terrain: Image.Image, lands_path: Path) -> Image.Image:
    lands = json.loads(lands_path.read_text(encoding="utf-8"))
    scale = ANTIALIAS
    overlay = Image.new("RGBA", (MAP_WIDTH * scale, SCREEN_HEIGHT * scale))
    shadow = Image.new("RGBA", overlay.size)
    shadow_draw = ImageDraw.Draw(shadow)
    line_draw = ImageDraw.Draw(overlay)

    for land in lands:
        points = land.get("points", [])
        if len(points) < 2:
            continue
        polygon = [(int(x * scale), int(y * scale)) for x, y in points]
        polygon.append(polygon[0])
        shadow_draw.line(polygon, fill=(0, 0, 0, 180), width=7, joint="curve")
        line_draw.line(polygon, fill=(171, 121, 48, 235), width=4, joint="curve")

    shadow = shadow.filter(ImageFilter.GaussianBlur(2.2))
    bordered = terrain.convert("RGBA")
    bordered.alpha_composite(shadow.resize(bordered.size, Image.Resampling.LANCZOS))
    bordered.alpha_composite(overlay.resize(bordered.size, Image.Resampling.LANCZOS))
    return bordered


def draw_frame(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int],
               *, strong: bool = False) -> None:
    """Draw the restrained iron-and-brass framing used by Reborn screens."""
    x1, y1, x2, y2 = box
    dark = (13, 17, 18, 230)
    iron = (62, 73, 74, 255)
    light = (119, 112, 87, 235) if strong else (89, 88, 73, 220)
    gold = (159, 125, 65, 245) if strong else (116, 94, 57, 225)
    draw.rectangle(box, outline=dark, width=5)
    draw.rectangle((x1 + 2, y1 + 2, x2 - 2, y2 - 2), outline=iron, width=2)
    draw.line((x1 + 5, y1 + 5, x2 - 5, y1 + 5), fill=light, width=1)
    draw.line((x1 + 5, y2 - 5, x2 - 5, y2 - 5), fill=gold, width=1)


def build_panel(texture_path: Path) -> Image.Image:
    """Compose the adventure HUD without changing any runtime hit boxes."""
    source = Image.open(texture_path).convert("RGB")
    crop = source.crop((760, 0, 1024, 768)).resize(
        (SCREEN_WIDTH - MAP_WIDTH, SCREEN_HEIGHT), Image.Resampling.LANCZOS
    )
    panel = Image.alpha_composite(
        crop.convert("RGBA"), Image.new("RGBA", crop.size, (2, 7, 9, 116))
    )
    draw = ImageDraw.Draw(panel, "RGBA")

    draw.rectangle((0, 0, 247, 767), outline=(12, 15, 16, 255), width=6)
    draw.line((3, 0, 3, 767), fill=(183, 139, 66, 245), width=2)
    draw.line((7, 0, 7, 767), fill=(65, 82, 82, 255), width=2)
    for y in (42, 112, 238, 414, 526, 580):
        draw.line((8, y, 245, y), fill=(10, 14, 15, 245), width=6)
        draw.line((10, y - 2, 243, y - 2), fill=(151, 117, 62, 230), width=1)
        draw.line((10, y + 2, 243, y + 2), fill=(73, 91, 91, 230), width=1)

    for row_y in (116, 176):
        for col in range(4):
            x1 = 8 + col * 60
            draw_frame(draw, (x1, row_y, x1 + 59, row_y + 59))

    draw_frame(draw, (46, 250, 194, 408), strong=True)
    draw.rectangle((51, 255, 189, 403), fill=(3, 11, 13, 150))
    draw_frame(draw, (7, 250, 38, 408))
    draw_frame(draw, (210, 250, 241, 408))

    draw.rectangle((10, 421, 243, 520), fill=(1, 8, 10, 142))
    draw_frame(draw, (10, 421, 243, 520))
    draw.rectangle((10, 534, 243, 574), fill=(2, 9, 11, 155))
    draw_frame(draw, (10, 534, 243, 574), strong=True)
    for y in (582, 642, 704):
        draw_frame(draw, (40, y, 137, min(y + 58, 766)))
        draw_frame(draw, (142, y, 239, min(y + 58, 766)))

    return panel


def main() -> None:
    args = parse_args()
    terrain = Image.open(args.terrain).convert("RGB")
    terrain = terrain.resize(
        (TERRAIN_FIT_WIDTH, TERRAIN_FIT_HEIGHT), Image.Resampling.LANCZOS
    )
    terrain = terrain.crop(
        (
            TERRAIN_CROP_X,
            TERRAIN_CROP_Y,
            TERRAIN_CROP_X + MAP_WIDTH,
            TERRAIN_CROP_Y + SCREEN_HEIGHT,
        )
    )
    terrain = draw_borders(terrain, args.lands)
    panel = build_panel(args.panel_texture)

    args.terrain_out.parent.mkdir(parents=True, exist_ok=True)
    terrain.save(args.terrain_out, optimize=True)

    if args.panel_out:
        args.panel_out.parent.mkdir(parents=True, exist_ok=True)
        panel.convert("RGB").save(args.panel_out, optimize=True)

    screen = Image.new("RGBA", (SCREEN_WIDTH, SCREEN_HEIGHT), (0, 0, 0, 255))
    screen.paste(terrain, (0, 0), terrain)
    screen.paste(panel, (MAP_WIDTH, 0), panel)

    args.screen_out.parent.mkdir(parents=True, exist_ok=True)
    screen.convert("RGB").save(args.screen_out, optimize=True)


if __name__ == "__main__":
    main()
