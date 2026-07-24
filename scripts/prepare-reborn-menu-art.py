#!/usr/bin/env python3
"""Prepare the shared Reborn roster and rune-summary background."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageEnhance


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    image = Image.open(args.source).convert("RGB")
    image = image.resize((1024, 768), Image.Resampling.LANCZOS)

    # UI text is pale gold, so keep the backdrop slightly restrained even
    # when users raise display brightness.
    image = ImageEnhance.Brightness(image).enhance(0.90)
    image = ImageEnhance.Contrast(image).enhance(0.96)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.output, optimize=True)
    print(f"Prepared {args.output}")


if __name__ == "__main__":
    main()
