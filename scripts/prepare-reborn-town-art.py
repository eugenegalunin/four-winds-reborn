#!/usr/bin/env python3
"""Prepare compact Reborn town sprites from transparent master artwork."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageFilter


TOWN_SIZE = 48
PADDING = 2


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    return parser.parse_args()


def prepare(source: Path, output: Path) -> None:
    image = Image.open(source).convert("RGBA")
    alpha = image.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        raise ValueError(f"town source has no visible pixels: {source}")

    image = image.crop(bbox)
    available = TOWN_SIZE - PADDING * 2
    scale = min(available / image.width, available / image.height)
    size = (max(1, round(image.width * scale)), max(1, round(image.height * scale)))
    image = image.resize(size, Image.Resampling.LANCZOS)
    image = image.filter(ImageFilter.UnsharpMask(radius=0.65, percent=115, threshold=2))

    canvas = Image.new("RGBA", (TOWN_SIZE, TOWN_SIZE))
    position = ((TOWN_SIZE - image.width) // 2, TOWN_SIZE - PADDING - image.height)
    canvas.alpha_composite(image, position)
    output.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output, optimize=True)


def main() -> None:
    args = parse_args()
    for clan in ("red", "yellow", "aqua", "purple"):
        prepare(
            args.source_dir / f"{clan}.png",
            args.output_dir / f"town_{clan}.png",
        )


if __name__ == "__main__":
    main()
