#!/usr/bin/env python3
"""Compile the Reborn five-stage turn timer into legacy atlas sprites."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageOps


ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "art" / "reborn" / "source" / "mahjong" / "markers" / "turn-delay"
SOURCE = SOURCE_DIR / "timer-ring.png"
COLORKEY = (255, 210, 210, 255)
TARGET_SIZE = (60, 60)

# Clockwise, starting at twelve o'clock.  The source is deliberately retained
# beside these derived sprites so future artwork can be recompiled exactly.
SOURCE_CROPS = (
    ((350, 55, 890, 375), False),
    ((55, 265, 410, 780), True),
    ((165, 775, 610, 1130), True),
    ((165, 775, 610, 1130), False),
    ((55, 265, 410, 780), False),
)


def trim_alpha(image: Image.Image) -> Image.Image:
    alpha = image.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        raise ValueError("Timer segment contains no visible pixels")
    return image.crop(bbox)


def fit_segment(image: Image.Image) -> Image.Image:
    image = trim_alpha(image)
    image.thumbnail((58, 58), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", TARGET_SIZE, COLORKEY)
    position = ((TARGET_SIZE[0] - image.width) // 2, (TARGET_SIZE[1] - image.height) // 2)
    canvas.alpha_composite(image, position)
    return canvas.convert("RGB")


def main() -> None:
    source = Image.open(SOURCE).convert("RGBA")
    for index, (crop, mirror) in enumerate(SOURCE_CROPS, start=1):
        source_segment = source.crop(crop)
        if mirror:
            source_segment = ImageOps.mirror(source_segment)
        segment = fit_segment(source_segment)
        output = SOURCE_DIR / f"turn_delay{index}.png"
        segment.save(output, optimize=False)
        print(f"Built {output.relative_to(ROOT)} ({segment.width}x{segment.height})")


if __name__ == "__main__":
    main()
