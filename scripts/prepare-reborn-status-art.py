#!/usr/bin/env python3
"""Build the Reborn island-status background from the Reborn terrain.

The status screen uses a fixed legacy panel layout.  We keep that geometry for
runtime compatibility, but replace the old island underneath it and recolour
the frame into Reborn's restrained bronze palette.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageEnhance, ImageFilter, ImageOps


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--terrain", type=Path, required=True)
    parser.add_argument("--layout", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    terrain = Image.open(args.terrain).convert("RGB")
    layout = Image.open(args.layout).convert("RGB")

    # The status view intentionally reads like a strategic parchment/relief,
    # not like the live coloured adventure map.
    terrain = terrain.resize(layout.size, Image.Resampling.LANCZOS)
    terrain = ImageOps.grayscale(terrain)
    terrain = ImageEnhance.Contrast(terrain).enhance(1.18)
    terrain = ImageEnhance.Brightness(terrain).enhance(0.62)
    terrain = terrain.filter(ImageFilter.GaussianBlur(0.35)).convert("RGB")

    out = terrain.copy()
    src = layout.load()
    dst = out.load()

    def in_frame_band(x: int, y: int) -> bool:
        # Fixed hit/layout geometry from the original status screen.  Keeping
        # the mask coordinate-based avoids mistaking coastlines for ornament.
        horizontal = any(abs(y - line) <= 5 for line in (2, 179, 353, 530, 707, 765))
        vertical = any(abs(x - line) <= 5 for line in (2, 188, 1021))
        bottom_button = y >= 704 and abs(x - 927) <= 5
        return horizontal or vertical or bottom_button

    for y in range(layout.height):
        for x in range(layout.width):
            if not in_frame_band(x, y):
                continue
            r, g, b = src[x, y]
            # The legacy separator is a yellow-green carved strip.  Preserve
            # its antialiased footprint, but translate it to iron and bronze.
            is_frame = r > 58 and g > 55 and b < 105 and (r + g) > (b * 1.7)
            if not is_frame:
                continue
            light = min(1.0, (r + g) / 390.0)
            dst[x, y] = (
                int(45 + 130 * light),
                int(40 + 91 * light),
                int(34 + 42 * light),
            )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    out.save(args.output, optimize=True)
    print(f"Prepared {args.output}")


if __name__ == "__main__":
    main()
