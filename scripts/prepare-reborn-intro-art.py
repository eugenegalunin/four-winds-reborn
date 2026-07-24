#!/usr/bin/env python3
"""Prepare the 17 Reborn intro masters for the in-game 1024x660 viewport."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageOps


FRAME_COUNT = 17
OUTPUT_SIZE = (1024, 660)


def prepare_frame(source: Path, frame: int) -> Image.Image:
    image = Image.open(source).convert("RGB")

    if frame == 17:
        # The final assembly is deliberately wider than the other masters.
        # Preserve every contender instead of cutting the figures at the edges.
        fitted = ImageOps.contain(image, OUTPUT_SIZE, Image.Resampling.LANCZOS)
        result = Image.new("RGB", OUTPUT_SIZE, (3, 5, 7))
        left = (OUTPUT_SIZE[0] - fitted.width) // 2
        top = (OUTPUT_SIZE[1] - fitted.height) // 2
        result.paste(fitted, (left, top))
        return result

    # The regular masters are 3:2. A tiny centred crop fills the slightly
    # wider game viewport while retaining the intended composition.
    return ImageOps.fit(
        image,
        OUTPUT_SIZE,
        method=Image.Resampling.LANCZOS,
        centering=(0.5, 0.5),
    )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--source-dir",
        type=Path,
        default=Path("art/reborn/source/intro"),
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("themes/reborn/assets/images"),
    )
    args = parser.parse_args()

    args.output_dir.mkdir(parents=True, exist_ok=True)

    for frame in range(1, FRAME_COUNT + 1):
        source = args.source_dir / f"introframe-{frame:02d}-master.png"
        output = args.output_dir / f"introframe_{frame}.png"
        if not source.is_file():
            raise FileNotFoundError(f"missing intro master: {source}")

        image = prepare_frame(source, frame)
        image.save(output, format="PNG", optimize=True)
        print(f"Prepared frame {frame:02d}: {source} -> {output}")


if __name__ == "__main__":
    main()
