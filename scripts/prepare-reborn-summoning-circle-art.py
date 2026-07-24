#!/usr/bin/env python3
"""Prepare the animated Reborn summoning-circle map sprites."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageEnhance, ImageFilter


SPRITE_SIZE = 64
PADDING = 2
FRAME_ANGLES = (0.0, 22.5, 45.0, 67.5)
FRAME_BRIGHTNESS = (0.92, 1.0, 1.08, 1.0)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    return parser.parse_args()


def prepare_master(source: Path) -> Image.Image:
    image = Image.open(source).convert("RGBA")
    bbox = image.getchannel("A").getbbox()
    if bbox is None:
        raise ValueError(f"summoning-circle source has no visible pixels: {source}")

    image = image.crop(bbox)
    available = SPRITE_SIZE - PADDING * 2
    scale = min(available / image.width, available / image.height)
    size = (max(1, round(image.width * scale)), max(1, round(image.height * scale)))
    image = image.resize(size, Image.Resampling.LANCZOS)

    canvas = Image.new("RGBA", (SPRITE_SIZE, SPRITE_SIZE))
    canvas.alpha_composite(
        image,
        ((SPRITE_SIZE - image.width) // 2, (SPRITE_SIZE - image.height) // 2),
    )
    return canvas


def main() -> None:
    args = parse_args()
    master = prepare_master(args.source)
    args.output_dir.mkdir(parents=True, exist_ok=True)

    for index, (angle, brightness) in enumerate(
        zip(FRAME_ANGLES, FRAME_BRIGHTNESS), start=1
    ):
        frame = master.rotate(
            -angle,
            resample=Image.Resampling.BICUBIC,
            expand=False,
        )
        frame = ImageEnhance.Brightness(frame).enhance(brightness)
        frame = frame.filter(
            ImageFilter.UnsharpMask(radius=0.55, percent=105, threshold=2)
        )
        frame.save(
            args.output_dir / f"summoning_circle_{index}.png",
            optimize=True,
        )


if __name__ == "__main__":
    main()
