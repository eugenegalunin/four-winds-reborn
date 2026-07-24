#!/usr/bin/env python3
"""Build the Reborn combat board without changing combat geometry.

The game renders units first and then places ``combatscreen.png`` over them.
The seven exact chroma-key openings below therefore form part of the UI
contract: changing them would hide creatures or alter selectable areas.
"""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageOps


SIZE = (1024, 768)
CHROMA = (255, 210, 210)
SLOTS = (
    (78, 198, 235, 337),
    (205, 394, 363, 534),
    (158, 589, 315, 728),
    (706, 192, 864, 332),
    (545, 393, 703, 533),
    (812, 595, 970, 735),
    (831, 393, 989, 533),
)


def cover(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    """Resize and centre-crop an image to *size*."""
    scale = max(size[0] / image.width, size[1] / image.height)
    resized = image.resize(
        (round(image.width * scale), round(image.height * scale)),
        Image.Resampling.LANCZOS,
    )
    left = (resized.width - size[0]) // 2
    top = (resized.height - size[1]) // 2
    return resized.crop((left, top, left + size[0], top + size[1]))


def stone_surface(source: Image.Image) -> Image.Image:
    base = cover(source.convert("RGB"), SIZE)
    base = ImageEnhance.Color(base).enhance(0.72)
    base = ImageEnhance.Contrast(base).enhance(1.08)
    shade = Image.new("RGBA", SIZE, (3, 8, 10, 62))
    return Image.alpha_composite(base.convert("RGBA"), shade)


def draw_line(draw: ImageDraw.ImageDraw, points, fill, width=1):
    draw.line(points, fill=fill, width=width, joint="curve")


def framed_panel(draw: ImageDraw.ImageDraw, box, radius=8, heavy=False):
    x0, y0, x1, y1 = box
    shadow = 7 if heavy else 4
    draw.rounded_rectangle(
        (x0 - shadow, y0 - shadow, x1 + shadow, y1 + shadow),
        radius=radius + shadow,
        fill=(2, 5, 6, 215),
        outline=(28, 36, 37, 255),
        width=2,
    )
    draw.rounded_rectangle(
        (x0 - 2, y0 - 2, x1 + 2, y1 + 2),
        radius=radius + 2,
        outline=(154, 117, 64, 255),
        width=2,
    )
    draw.rounded_rectangle(
        (x0 - 5, y0 - 5, x1 + 5, y1 + 5),
        radius=radius + 5,
        outline=(57, 75, 77, 255),
        width=2,
    )


def build_background(source: Image.Image) -> Image.Image:
    result = stone_surface(source)

    # Vignette keeps combatants readable while retaining the stone texture.
    vignette = Image.new("L", SIZE, 0)
    vd = ImageDraw.Draw(vignette)
    vd.ellipse((-170, -220, 1194, 1010), fill=210)
    vignette = vignette.filter(ImageFilter.GaussianBlur(115))
    black = Image.new("RGBA", SIZE, (0, 0, 0, 155))
    black.putalpha(ImageOps.invert(vignette))
    result = Image.alpha_composite(result, black)

    overlay = Image.new("RGBA", SIZE, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)

    # Header: portraits remain separate runtime sprites at both ends.
    draw.rounded_rectangle((2, 2, 1021, 151), radius=12,
                           fill=(4, 10, 12, 190), outline=(76, 97, 98, 255), width=2)
    draw.rectangle((151, 8, 872, 103), fill=(5, 10, 12, 180))
    draw.line((151, 104, 872, 104), fill=(158, 119, 62, 255), width=2)
    draw.line((151, 183, 872, 183), fill=(55, 75, 77, 235), width=2)
    framed_panel(draw, (9, 8, 149, 148), radius=5)
    framed_panel(draw, (877, 8, 1017, 148), radius=5)

    # Two armies get different restrained accents: warm attack, cold defence.
    attacker = (157, 78, 47, 62)
    defender = (60, 126, 145, 58)
    warm_line = (183, 102, 57, 215)
    cold_line = (69, 142, 158, 215)
    draw.polygon(((0, 158), (470, 158), (391, 768), (0, 768)), fill=attacker)
    draw.polygon(((1024, 158), (554, 158), (633, 768), (1024, 768)), fill=defender)

    # Central neutral spine separates sides and supports the timeline visually.
    draw.polygon(((494, 157), (530, 157), (548, 661), (512, 701), (476, 661)),
                 fill=(4, 9, 11, 205), outline=(75, 91, 91, 240))
    for y in range(218, 650, 72):
        draw.regular_polygon((512, y, 10), 4, rotation=45,
                             fill=(16, 25, 27, 255), outline=(160, 120, 62, 235))

    # Subtle paths make the seven combat positions read as one arena.
    left_centres = ((156, 267), (284, 464), (236, 658))
    right_centres = ((785, 262), (624, 463), (891, 665))
    for centres, color in ((left_centres, warm_line), (right_centres, cold_line)):
        for a, b in zip(centres, centres[1:]):
            draw_line(draw, (a, b), color, 3)
        for centre in centres:
            draw.line((centre[0], centre[1], 512, 385), fill=color, width=2)
    draw.line((909, 463, 970, 463), fill=cold_line, width=3)

    # Unit openings: retain exact chroma-key rectangles, build frames outside.
    for slot in SLOTS:
        framed_panel(draw, slot, radius=5, heavy=True)
        x0, y0, x1, y1 = slot
        # Corner rivets stay outside the runtime creature image.
        for x, y in ((x0 - 4, y0 - 4), (x1 + 3, y0 - 4),
                     (x0 - 4, y1 + 3), (x1 + 3, y1 + 3)):
            draw.ellipse((x - 3, y - 3, x + 3, y + 3),
                         fill=(177, 134, 70, 255), outline=(24, 31, 32, 255))
        draw.rectangle(slot, fill=CHROMA + (255,))

    # Bottom information/choice strip. Runtime text and controls sit on top.
    draw.rounded_rectangle((315, 650, 709, 766), radius=10,
                           fill=(3, 8, 10, 226), outline=(73, 94, 95, 255), width=2)
    draw.line((331, 682, 693, 682), fill=(154, 116, 61, 235), width=1)
    draw.rounded_rectangle((398, 712, 626, 760), radius=7,
                           fill=(10, 18, 20, 220), outline=(154, 116, 61, 240), width=2)

    return Image.alpha_composite(result, overlay).convert("RGB")


def build_cellfill(source: Image.Image) -> Image.Image:
    surface = cover(source.convert("RGB"), (157, 140))
    surface = ImageEnhance.Brightness(surface).enhance(0.48)
    surface = ImageEnhance.Color(surface).enhance(0.55)
    result = Image.new("RGB", (176, 140), (8, 14, 15))
    result.paste(surface, (0, 0))
    draw = ImageDraw.Draw(result, "RGBA")
    draw.rectangle((0, 0, 156, 139), fill=(2, 8, 10, 92),
                   outline=(126, 97, 56, 255), width=2)
    draw.ellipse((41, 24, 116, 99), outline=(102, 79, 48, 135), width=2)
    draw.ellipse((53, 36, 104, 87), outline=(61, 88, 91, 125), width=1)
    draw.regular_polygon((78, 62, 17), 4, rotation=45,
                         outline=(145, 109, 61, 125))

    # The rightmost crop is the loyalty meter drawn beside live units.
    draw.rectangle((157, 0, 175, 139), fill=(5, 9, 10, 255))
    draw.rectangle((159, 2, 173, 137), fill=(15, 22, 23, 255),
                   outline=(170, 128, 67, 255), width=2)
    draw.line((162, 5, 162, 134), fill=(74, 94, 95, 230), width=1)
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--background-out", type=Path, required=True)
    parser.add_argument("--cellfill-out", type=Path, required=True)
    args = parser.parse_args()

    source = Image.open(args.source)
    args.background_out.parent.mkdir(parents=True, exist_ok=True)
    args.cellfill_out.parent.mkdir(parents=True, exist_ok=True)
    build_background(source).save(args.background_out, optimize=True)
    build_cellfill(source).save(args.cellfill_out, optimize=True)


if __name__ == "__main__":
    main()
