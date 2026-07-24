#!/usr/bin/env python3
"""Build compact Reborn clan emblems from the generated master artwork."""

from __future__ import annotations

from pathlib import Path
import math

from PIL import Image, ImageDraw, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "art/reborn/source/clans/components"
OUTPUT = ROOT / "themes/reborn/assets/images"

EMBLEMS = {
    "red": "clan_red.png",
    "yellow": "clan_yellow.png",
    "aqua": "clan_aqua.png",
    "purple": "clan_purple.png",
}

BADGES = {
    "red": "clan_badge_red.png",
    "yellow": "clan_badge_yellow.png",
    "aqua": "clan_badge_aqua.png",
    "purple": "clan_badge_purple.png",
}

BADGE_ACCENTS = {
    "red": (157, 54, 49),
    "yellow": (210, 163, 58),
    "aqua": (70, 169, 185),
    "purple": (137, 78, 167),
}

CANVAS = (140, 140)
CONTENT = (106, 106)
BADGE_SIZE = 57
BADGE_SCALE = 4


def build_card_background() -> Image.Image:
    """Opaque clan card that covers the legacy robes baked into screen1."""
    width, height = CANVAS
    result = Image.new("RGBA", CANVAS, (8, 12, 13, 255))
    pixels = result.load()
    center_x = (width - 1) / 2
    center_y = (height - 1) / 2
    max_distance = math.hypot(center_x, center_y)

    for y in range(height):
        for x in range(width):
            distance = math.hypot(x - center_x, y - center_y) / max_distance
            glow = max(0.0, 1.0 - distance)
            grain = ((x * 17 + y * 31) % 9) - 4
            pixels[x, y] = (
                max(0, min(255, int(8 + glow * 11 + grain * 0.35))),
                max(0, min(255, int(12 + glow * 20 + grain * 0.45))),
                max(0, min(255, int(13 + glow * 17 + grain * 0.35))),
                255,
            )

    # The selection screen supplies the outer frame; this restrained inner
    # line gives the new emblems a deliberate card edge at every use site.
    for inset, color in ((1, (92, 76, 45, 255)), (2, (42, 51, 46, 255))):
        for x in range(inset, width - inset):
            pixels[x, inset] = color
            pixels[x, height - inset - 1] = color
        for y in range(inset, height - inset):
            pixels[inset, y] = color
            pixels[width - inset - 1, y] = color

    return result


def build_emblem(source: Path) -> Image.Image:
    image = Image.open(source).convert("RGBA")
    bounds = image.getchannel("A").getbbox()
    if not bounds:
        raise RuntimeError(f"empty clan emblem: {source}")

    image = image.crop(bounds)
    image.thumbnail(CONTENT, Image.Resampling.LANCZOS)

    result = build_card_background()
    position = ((CANVAS[0] - image.width) // 2, (CANVAS[1] - image.height) // 2)
    result.alpha_composite(image, position)
    return result


def build_badge(source: Path, accent: tuple[int, int, int]) -> Image.Image:
    """Render a compact round clan medallion for summaries and map dialogs."""
    scale = BADGE_SCALE
    size = BADGE_SIZE * scale
    result = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(result)

    # A dark metal coin with a restrained clan-colour inner glow. Drawing at
    # 4x and downsampling keeps the 57px circle and its emblem crisp.
    draw.ellipse(
        (2 * scale, 2 * scale, size - 2 * scale - 1, size - 2 * scale - 1),
        fill=(5, 8, 10, 248),
        outline=(31, 38, 39, 255),
        width=2 * scale,
    )
    draw.ellipse(
        (4 * scale, 4 * scale, size - 4 * scale - 1, size - 4 * scale - 1),
        outline=(145, 118, 68, 255),
        width=2 * scale,
    )
    draw.ellipse(
        (7 * scale, 7 * scale, size - 7 * scale - 1, size - 7 * scale - 1),
        fill=(8, 13, 15, 255),
        outline=(*accent, 230),
        width=2 * scale,
    )

    image = Image.open(source).convert("RGBA")
    bounds = image.getchannel("A").getbbox()
    if not bounds:
        raise RuntimeError(f"empty clan emblem: {source}")

    image = image.crop(bounds)
    image.thumbnail((39 * scale, 39 * scale), Image.Resampling.LANCZOS)
    position = ((size - image.width) // 2, (size - image.height) // 2)

    shadow = Image.new("RGBA", result.size, (0, 0, 0, 0))
    shadow_alpha = image.getchannel("A").filter(ImageFilter.GaussianBlur(2 * scale))
    shadow_image = Image.new("RGBA", image.size, (*accent, 95))
    shadow_image.putalpha(shadow_alpha)
    shadow.alpha_composite(shadow_image, position)
    result = Image.alpha_composite(result, shadow)
    result.alpha_composite(image, position)

    return result.resize((BADGE_SIZE, BADGE_SIZE), Image.Resampling.LANCZOS)


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for clan, filename in EMBLEMS.items():
        source = SOURCE / f"{clan}.png"
        target = OUTPUT / filename
        build_emblem(source).save(target, optimize=True)
        print(target.relative_to(ROOT))

        badge_target = OUTPUT / BADGES[clan]
        build_badge(source, BADGE_ACCENTS[clan]).save(badge_target, optimize=True)
        print(badge_target.relative_to(ROOT))


if __name__ == "__main__":
    main()
