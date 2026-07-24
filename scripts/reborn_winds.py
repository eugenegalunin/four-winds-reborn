#!/usr/bin/env python3
"""Compile the Reborn wind glyph sheet into legacy 40x48 atlas sprites."""

from __future__ import annotations

from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIR = ROOT / "art" / "reborn" / "source" / "mahjong" / "markers" / "winds"
SOURCE = SOURCE_DIR / "wind-glyphs.png"
COLORKEY = (255, 210, 210, 255)
TARGET_SIZE = (40, 48)
WINDS = ("east", "south", "west", "north")
STATES = ("blue", "yellow")


def trim_alpha(image: Image.Image) -> Image.Image:
    bbox = image.getchannel("A").getbbox()
    if bbox is None:
        raise ValueError("Wind glyph cell contains no visible pixels")
    return image.crop(bbox)


def fit_glyph(image: Image.Image) -> Image.Image:
    image = trim_alpha(image)
    image.thumbnail((38, 46), Image.Resampling.LANCZOS)
    canvas = Image.new("RGBA", TARGET_SIZE, COLORKEY)
    position = ((TARGET_SIZE[0] - image.width) // 2, (TARGET_SIZE[1] - image.height) // 2)
    canvas.alpha_composite(image, position)
    return canvas.convert("RGB")


def main() -> None:
    source = Image.open(SOURCE).convert("RGBA")
    x_edges = (0, source.width // 4, source.width // 2, source.width * 3 // 4, source.width)
    y_edges = (0, source.height // 2, source.height)

    for row, state in enumerate(STATES):
        for column, wind in enumerate(WINDS):
            crop = (x_edges[column], y_edges[row], x_edges[column + 1], y_edges[row + 1])
            output = SOURCE_DIR / f"wind_{state}_{wind}.png"
            fit_glyph(source.crop(crop)).save(output, optimize=False)
            print(f"Built {output.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
