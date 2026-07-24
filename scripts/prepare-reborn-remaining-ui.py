#!/usr/bin/env python3
"""Build the last legacy-shaped Reborn UI and atlas assets.

The engine still expects ten bitmaps with classic dimensions and crop
coordinates.  This builder keeps those runtime contracts while creating every
visible pixel from Reborn source art or deterministic drawing primitives.
"""

from __future__ import annotations

import argparse
import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter, ImageOps


CHROMA = (255, 210, 210)
INK = (5, 9, 11)
PANEL = (9, 15, 17)
PANEL_LIGHT = (17, 28, 30)
BRASS = (145, 112, 59)
BRASS_LIGHT = (219, 185, 112)
CYAN = (58, 199, 209)
EMBER = (224, 75, 31)
VIOLET = (137, 71, 180)
PARCHMENT = (205, 198, 171)


def _root() -> Path:
    return Path(__file__).resolve().parents[1]


def _noise_texture(size: tuple[int, int], seed: int, base: tuple[int, int, int]) -> Image.Image:
    rng = random.Random(seed)
    width, height = size
    small = Image.new("RGB", (max(1, width // 4), max(1, height // 4)), base)
    pixels = small.load()
    for y in range(small.height):
        for x in range(small.width):
            grain = rng.randint(-12, 12)
            pixels[x, y] = tuple(max(0, min(255, channel + grain)) for channel in base)
    return small.resize(size, Image.Resampling.BILINEAR).filter(ImageFilter.GaussianBlur(0.6))


def _cover(image: Image.Image, size: tuple[int, int], focus_y: float = 0.5) -> Image.Image:
    source = image.convert("RGB")
    scale = max(size[0] / source.width, size[1] / source.height)
    resized = source.resize(
        (max(1, round(source.width * scale)), max(1, round(source.height * scale))),
        Image.Resampling.LANCZOS,
    )
    left = max(0, (resized.width - size[0]) // 2)
    excess_y = max(0, resized.height - size[1])
    top = round(excess_y * max(0.0, min(1.0, focus_y)))
    return resized.crop((left, top, left + size[0], top + size[1]))


def _panel(draw: ImageDraw.ImageDraw, box: tuple[int, int, int, int], *, cyan: bool = False) -> None:
    x0, y0, x1, y1 = box
    draw.rectangle(box, fill=PANEL, outline=(40, 48, 46), width=1)
    draw.rectangle((x0 + 2, y0 + 2, x1 - 2, y1 - 2), outline=BRASS, width=1)
    accent = CYAN if cyan else BRASS_LIGHT
    length = max(10, min(30, (x1 - x0) // 5))
    for sx, sy, dx, dy in (
        (x0 + 3, y0 + 3, 1, 0),
        (x1 - 3, y0 + 3, -1, 0),
        (x0 + 3, y1 - 3, 1, 0),
        (x1 - 3, y1 - 3, -1, 0),
    ):
        draw.line((sx, sy, sx + dx * length, sy + dy * length), fill=accent, width=1)


def _rune_ring(
    image: Image.Image,
    center: tuple[int, int],
    radius: int,
    color: tuple[int, int, int],
    *,
    phase: float = 0.0,
    width: int = 3,
) -> None:
    overlay = Image.new("RGBA", image.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    cx, cy = center
    draw.ellipse((cx - radius, cy - radius, cx + radius, cy + radius), outline=color + (210,), width=width)
    draw.ellipse(
        (cx - radius + 5, cy - radius + 5, cx + radius - 5, cy + radius - 5),
        outline=BRASS_LIGHT + (150,),
        width=1,
    )
    for index in range(8):
        angle = phase + index * math.tau / 8
        inner = radius - 8
        outer = radius + 1
        x1 = cx + math.cos(angle) * inner
        y1 = cy + math.sin(angle) * inner
        x2 = cx + math.cos(angle) * outer
        y2 = cy + math.sin(angle) * outer
        draw.line((x1, y1, x2, y2), fill=color + (220,), width=2)
    glow = overlay.filter(ImageFilter.GaussianBlur(max(2, radius // 8)))
    image.alpha_composite(glow)
    image.alpha_composite(overlay)


def _flatten_key(image: Image.Image, threshold: int = 7) -> Image.Image:
    rgba = image.convert("RGBA")
    matte = Image.new("RGBA", rgba.size, INK + (255,))
    visible = Image.alpha_composite(matte, rgba).convert("RGB")
    mask = rgba.getchannel("A").point(lambda value: 255 if value > threshold else 0)
    output = Image.new("RGB", rgba.size, CHROMA)
    output.paste(visible, (0, 0), mask)
    return output


def _paste_keyed(canvas: Image.Image, sprite: Image.Image, position: tuple[int, int]) -> None:
    rgba = sprite.convert("RGBA")
    canvas.paste(rgba, position, rgba)


def build_cast_informer(output: Path, state: str = "ready") -> None:
    if state not in {"idle", "pressed", "ready"}:
        raise ValueError(f"Unsupported cast button state: {state}")

    ready = state == "ready"
    pressed = state == "pressed"
    image = Image.new("RGB", (32, 84), INK)
    draw = ImageDraw.Draw(image)
    shift = 1 if pressed else 0
    frame = (3, 1 + shift, 28, 82)
    frame_fill = (15, 22, 23) if pressed else (12, 20, 22)
    frame_outline = BRASS_LIGHT if pressed else BRASS
    draw.rounded_rectangle(frame, radius=5, fill=frame_fill, outline=frame_outline, width=2)
    draw.line((8, 70 + shift, 24, 70 + shift), fill=BRASS_LIGHT if ready else BRASS, width=1)
    draw.polygon(
        ((16, 7 + shift), (23, 14 + shift), (16, 20 + shift), (9, 14 + shift)),
        fill=(17, 33, 36) if ready else (24, 28, 28),
        outline=BRASS_LIGHT if pressed else BRASS,
    )
    for y in range(26, 68):
        strength = 1.0 - abs(y - 47) / 23
        if ready:
            color = (
                round(20 + 35 * strength),
                round(65 + 145 * strength),
                round(72 + 150 * strength),
            )
        elif pressed:
            color = (
                round(35 + 38 * strength),
                round(39 + 30 * strength),
                round(38 + 18 * strength),
            )
        else:
            color = (
                round(18 + 18 * strength),
                round(30 + 28 * strength),
                round(32 + 30 * strength),
            )
        draw.line((11, y + shift, 21, y + shift), fill=color)
    gem_outline = CYAN if ready else BRASS
    draw.ellipse((12, 42 + shift, 20, 50 + shift), fill=(4, 12, 13), outline=gem_outline)
    if ready:
        draw.ellipse((15, 45 + shift, 17, 47 + shift), fill=(230, 251, 236))
    elif pressed:
        draw.ellipse((15, 45 + shift, 17, 47 + shift), fill=BRASS_LIGHT)
    image.save(output)


def build_charbio9(source: Path, output: Path) -> Image.Image:
    portrait = _cover(Image.open(source), (264, 516), focus_y=0.48)
    portrait = ImageEnhance.Contrast(portrait).enhance(1.06)
    draw = ImageDraw.Draw(portrait)
    draw.rectangle((0, 0, 263, 515), outline=(71, 60, 44), width=2)
    draw.rectangle((4, 4, 259, 511), outline=BRASS, width=1)
    portrait.save(output)
    return portrait


def build_charselect(charbio: Image.Image, output: Path) -> None:
    image = _noise_texture((1024, 768), 4271, (5, 9, 10))
    draw = ImageDraw.Draw(image)

    # Header and identity block.
    _panel(draw, (317, 4, 807, 75), cyan=True)
    _panel(draw, (808, 4, 1019, 75))

    # Portrait grid.
    for y in (15, 164, 313, 462, 611):
        for x in (15, 164):
            _panel(draw, (x, y, x + 143, y + 143), cyan=True)
            draw.rectangle((x + 6, y + 6, x + 137, y + 137), fill=(14, 22, 23))
            draw.line((x + 9, y + 132, x + 132, y + 9), fill=(20, 55, 58), width=1)

    # Unknown portrait is baked into the one slot that is cropped as port0.
    unknown = _cover(charbio, (132, 132), focus_y=0.18)
    image.paste(unknown, (169, 617))

    # Selected biography and text regions.
    parchment = _noise_texture((266, 518), 8817, PARCHMENT)
    parchment = ImageEnhance.Contrast(parchment).enhance(0.82)
    image.paste(parchment, (321, 79))
    draw.rectangle((320, 78, 587, 598), outline=BRASS_LIGHT, width=2)
    draw.rectangle((324, 82, 583, 594), outline=(79, 62, 38), width=1)
    draw.rectangle((320, 598, 587, 610), fill=INK, outline=BRASS)
    _panel(draw, (588, 78, 1019, 610))

    # Clan cards and controls.
    for x in (329, 477, 626, 775):
        _panel(draw, (x, 611, x + 143, 755), cyan=True)
        draw.ellipse((x + 28, 629, x + 115, 716), outline=(38, 77, 78), width=2)
    _panel(draw, (919, 611, 1019, 755))

    # Subtle radial engravings keep empty regions intentional.
    for cx, cy, radius in ((510, 345, 92), (804, 343, 142)):
        draw.ellipse((cx - radius, cy - radius, cx + radius, cy + radius), outline=(25, 41, 42), width=1)
        for index in range(12):
            angle = index * math.tau / 12
            draw.line(
                (
                    cx + math.cos(angle) * (radius - 8),
                    cy + math.sin(angle) * (radius - 8),
                    cx + math.cos(angle) * radius,
                    cy + math.sin(angle) * radius,
                ),
                fill=(42, 53, 49),
                width=1,
            )
    image.save(output)


def build_charselectoverlay(output: Path) -> None:
    image = Image.new("RGB", (280, 140), CHROMA)
    draw = ImageDraw.Draw(image)

    # Disabled portrait veil.
    draw.rectangle((0, 0, 139, 139), fill=(13, 22, 24))
    for position in range(-140, 280, 12):
        draw.line((position, 0, position - 140, 140), fill=(25, 44, 47), width=3)
    draw.rectangle((2, 2, 137, 137), outline=(54, 73, 72), width=2)
    draw.ellipse((45, 44, 95, 94), outline=(79, 91, 85), width=2)
    draw.line((54, 52, 86, 86), fill=(79, 91, 85), width=3)

    # Selection brackets. The runtime crops the top and bottom 25px strips.
    for top in (0, 115):
        draw.rectangle((142, top + 2, 277, top + 22), outline=BRASS, width=2)
        draw.line((149, top + 7, 181, top + 7), fill=CYAN, width=2)
        draw.line((270, top + 17, 238, top + 17), fill=CYAN, width=2)
        draw.polygon(
            ((143, top + 12), (150, top + 5), (157, top + 12), (150, top + 19)),
            fill=BRASS_LIGHT,
        )
        draw.polygon(
            ((277, top + 12), (270, top + 5), (263, top + 12), (270, top + 19)),
            fill=BRASS_LIGHT,
        )
    image.save(output)


def build_chatbox(output: Path) -> None:
    image = _noise_texture((776, 236), 9314, PANEL)
    draw = ImageDraw.Draw(image)
    _panel(draw, (0, 0, 775, 235), cyan=True)
    draw.rectangle((8, 8, 767, 181), fill=(6, 12, 14), outline=(59, 68, 62), width=1)
    draw.line((13, 187, 762, 187), fill=BRASS, width=1)
    for x in range(30, 747, 72):
        draw.polygon(((x, 211), (x + 7, 204), (x + 14, 211), (x + 7, 218)), outline=(30, 80, 84))
    draw.ellipse((744, 194, 765, 225), outline=BRASS, width=2)
    draw.line((754, 199, 754, 220), fill=CYAN, width=2)
    image.save(output)


def build_listspell(output: Path) -> None:
    image = _noise_texture((768, 516), 1182, (6, 11, 13))
    draw = ImageDraw.Draw(image)
    _panel(draw, (0, 0, 767, 515), cyan=True)
    draw.rectangle((8, 8, 759, 48), fill=(11, 20, 22), outline=BRASS, width=1)
    draw.line((383, 50, 383, 474), fill=BRASS_LIGHT, width=2)
    for top in range(55, 465, 34):
        draw.rectangle((8, top, 378, top + 32), fill=(9, 16, 18), outline=(77, 66, 44), width=1)
        draw.rectangle((389, top, 759, top + 32), fill=(9, 16, 18), outline=(77, 66, 44), width=1)
        draw.line((18, top + 27, 368, top + 27), fill=(16, 52, 56), width=1)
        draw.line((399, top + 27, 749, top + 27), fill=(16, 52, 56), width=1)
    draw.rectangle((8, 476, 759, 508), fill=(10, 18, 20), outline=BRASS, width=1)
    for x in (93, 473):
        draw.ellipse((x, 15, x + 18, 33), outline=(44, 112, 117), width=1)
    image.save(output)


def _effect_strip(kind: int, output: Path) -> None:
    strip = Image.new("RGBA", (700, 140), (0, 0, 0, 0))
    for frame_index in range(5):
        frame = Image.new("RGBA", (140, 140), (0, 0, 0, 0))
        progress = frame_index / 4
        center = (70, 70)
        if kind == 4:
            radius = round(24 + progress * 30)
            _rune_ring(frame, center, radius, CYAN, phase=progress * 1.7, width=3)
            draw = ImageDraw.Draw(frame)
            shield = [
                (70, 28 - round(progress * 3)),
                (104, 43),
                (96, 91),
                (70, 112 + round(progress * 3)),
                (44, 91),
                (36, 43),
            ]
            draw.polygon(shield, fill=(14, 35, 40, 95), outline=BRASS_LIGHT + (235,))
            draw.line((70, 36, 70, 101), fill=CYAN + (225,), width=3)
        elif kind == 5:
            draw = ImageDraw.Draw(frame)
            angle = -0.85 + progress * 1.55
            length = 82
            x1 = 70 - math.cos(angle) * length / 2
            y1 = 70 - math.sin(angle) * length / 2
            x2 = 70 + math.cos(angle) * length / 2
            y2 = 70 + math.sin(angle) * length / 2
            glow = Image.new("RGBA", frame.size, (0, 0, 0, 0))
            glow_draw = ImageDraw.Draw(glow)
            glow_draw.line((x1, y1, x2, y2), fill=EMBER + (180,), width=18)
            frame.alpha_composite(glow.filter(ImageFilter.GaussianBlur(10)))
            draw.line((x1, y1, x2, y2), fill=(255, 198, 95, 245), width=5)
            for spark in range(7):
                theta = angle + math.pi / 2 + (spark - 3) * 0.13
                offset = 16 + spark * 4
                sx = 70 + math.cos(theta) * offset
                sy = 70 + math.sin(theta) * offset
                draw.ellipse((sx - 2, sy - 2, sx + 2, sy + 2), fill=EMBER + (220,))
        else:
            draw = ImageDraw.Draw(frame)
            radius = round(14 + progress * 43)
            _rune_ring(frame, center, radius, VIOLET, phase=-progress * 2.1, width=2)
            for arm in range(5):
                points = []
                base = arm * math.tau / 5 - progress * 0.8
                for step in range(15):
                    distance = 8 + step * (radius / 14)
                    angle = base + step * 0.12 + math.sin(progress * math.tau + step) * 0.03
                    points.append((70 + math.cos(angle) * distance, 70 + math.sin(angle) * distance))
                draw.line(points, fill=(203, 104, 255, 230), width=3)
            draw.ellipse((61, 61, 79, 79), fill=(2, 7, 10, 240), outline=CYAN + (230,), width=2)
        strip.alpha_composite(frame, (frame_index * 140, 0))
    _flatten_key(strip).save(output)


def _icon_tile(size: tuple[int, int], index: int, family: str) -> Image.Image:
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    width, height = size
    cx, cy = width // 2, height // 2
    palette = (CYAN, EMBER, VIOLET, BRASS_LIGHT)
    color = palette[index % len(palette)]
    if family == "skill":
        draw.ellipse((2, 2, width - 3, height - 3), fill=(8, 15, 17, 235), outline=BRASS + (255,), width=2)
        points = []
        lobes = 3 + index % 4
        for step in range(lobes * 2):
            angle = -math.pi / 2 + step * math.pi / lobes
            radius = min(width, height) * (0.34 if step % 2 == 0 else 0.14)
            points.append((cx + math.cos(angle) * radius, cy + math.sin(angle) * radius))
        draw.polygon(points, outline=color + (255,))
        draw.ellipse((cx - 2, cy - 2, cx + 2, cy + 2), fill=(235, 232, 199, 255))
    else:
        draw.ellipse((1, 1, width - 2, height - 2), fill=(6, 12, 14, 220), outline=(48, 61, 58, 255))
        turns = 2 + index % 3
        points = []
        for step in range(18):
            angle = index * 0.7 + step * math.tau * turns / 17
            radius = 2 + step * (min(width, height) * 0.38 / 17)
            points.append((cx + math.cos(angle) * radius, cy + math.sin(angle) * radius))
        draw.line(points, fill=color + (255,), width=2)
    return image


def _score_icon(index: int) -> Image.Image:
    image = Image.new("RGBA", (57, 57), (0, 0, 0, 0))
    _rune_ring(image, (28, 28), 24, (CYAN, BRASS_LIGHT, EMBER, VIOLET, CYAN)[index], phase=index * 0.4, width=2)
    draw = ImageDraw.Draw(image)
    if index == 0:
        draw.polygon(((28, 10), (34, 24), (49, 28), (34, 34), (28, 49), (22, 34), (8, 28), (22, 24)), fill=BRASS_LIGHT + (230,))
    elif index == 1:
        draw.polygon(((12, 39), (18, 21), (28, 31), (37, 14), (46, 39)), outline=CYAN + (255,))
    elif index == 2:
        draw.ellipse((15, 15, 41, 41), outline=EMBER + (255,), width=3)
        draw.line((28, 9, 28, 47), fill=BRASS_LIGHT + (255,), width=2)
    elif index == 3:
        for x in (17, 28, 39):
            draw.line((x, 42, x, 18), fill=VIOLET + (255,), width=4)
    else:
        draw.arc((12, 12, 44, 44), 35, 320, fill=CYAN + (255,), width=4)
        draw.ellipse((24, 24, 32, 32), fill=BRASS_LIGHT + (255,))
    return image


def _button_icon(index: int) -> Image.Image:
    image = Image.new("RGBA", (57, 57), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.ellipse((3, 3, 53, 53), fill=(7, 13, 15, 248), outline=BRASS + (255,), width=3)
    draw.ellipse((8, 8, 48, 48), outline=(46, 87, 88, 255), width=2)
    if index == 0:
        draw.line((17, 30, 27, 40, 43, 18), fill=BRASS_LIGHT + (255,), width=4)
    elif index == 1:
        draw.line((16, 18, 41, 43), fill=EMBER + (255,), width=4)
        draw.line((41, 18, 16, 43), fill=EMBER + (255,), width=4)
    elif index == 2:
        draw.ellipse((16, 16, 41, 41), outline=CYAN + (255,), width=3)
        draw.ellipse((26, 26, 31, 31), fill=BRASS_LIGHT + (255,))
    else:
        draw.polygon(((17, 16), (42, 28), (17, 41)), fill=VIOLET + (255,), outline=BRASS_LIGHT + (255,))
    return image


def build_map_atlas(output: Path) -> None:
    atlas = Image.new("RGB", (1024, 1159), CHROMA)

    # Four Winds tower portrait, used independently from the adventure map.
    tower = Image.new("RGBA", (140, 140), (0, 0, 0, 0))
    tower_draw = ImageDraw.Draw(tower)
    tower_draw.ellipse((8, 8, 132, 132), fill=(6, 12, 14, 242), outline=BRASS + (255,), width=3)
    for level, width in ((105, 58), (82, 45), (60, 34), (40, 21)):
        x0 = 70 - width // 2
        x1 = 70 + width // 2
        tower_draw.polygon(((x0, level), (x1, level), (x1 - 6, level - 28), (x0 + 6, level - 28)), fill=(26, 31, 32, 255), outline=BRASS_LIGHT + (255,))
    tower_draw.line((70, 20, 70, 116), fill=CYAN + (255,), width=3)
    tower_draw.ellipse((64, 14, 76, 26), fill=(225, 246, 230, 255))
    atlas.paste(_flatten_key(tower), (344, 310))

    # Skills and spells keep the exact fixed atlas positions.
    for index in range(8):
        atlas.paste(_flatten_key(_icon_tile((26, 26), index, "skill")), (288 + index * 26, 768))
    for index in range(19):
        atlas.paste(_flatten_key(_icon_tile((20, 20), index, "spell")), (640 + index * 20, 768))

    for index in range(4):
        atlas.paste(_flatten_key(_button_icon(index)), (684 + index * 57, 911))
    score_positions = ((114, 3), (171, 1), (228, 2), (285, 0), (342, 4))
    for x, index in score_positions:
        atlas.paste(_flatten_key(_score_icon(index)), (x, 912))

    # Fight marker.
    fight = Image.new("RGBA", (80, 46), (0, 0, 0, 0))
    fight_draw = ImageDraw.Draw(fight)
    fight_draw.ellipse((18, 2, 62, 44), fill=(8, 14, 16, 235), outline=BRASS + (255,), width=2)
    fight_draw.line((25, 35, 54, 9), fill=EMBER + (255,), width=5)
    fight_draw.line((26, 9, 55, 35), fill=CYAN + (255,), width=5)
    fight_draw.ellipse((35, 18, 45, 28), fill=BRASS_LIGHT + (255,))
    atlas.paste(_flatten_key(fight), (566, 975))

    # Small animated volcanic map markers.
    for index, x in enumerate((318, 350, 382)):
        lava = Image.new("RGBA", (32, 38), (0, 0, 0, 0))
        draw = ImageDraw.Draw(lava)
        lift = (0, 4, 1)[index]
        draw.polygon(((4, 35), (9, 18 + lift), (16, 7 - lift), (22, 21), (28, 35)), fill=(61, 22, 17, 245), outline=BRASS + (220,))
        draw.line((11, 29, 16, 15 - lift, 20, 29), fill=EMBER + (255,), width=4)
        atlas.paste(_flatten_key(lava), (x, 1018))
    for index, x in enumerate((318, 353, 388)):
        lava = Image.new("RGBA", (35, 35), (0, 0, 0, 0))
        draw = ImageDraw.Draw(lava)
        radius = 8 + index * 4
        draw.ellipse((17 - radius, 17 - radius, 17 + radius, 17 + radius), fill=EMBER + (80 + index * 45,), outline=BRASS_LIGHT + (220,), width=2)
        atlas.paste(_flatten_key(lava), (x, 1056))
    for width, height, y, xs in (
        (23, 20, 1018, (414, 437, 460)),
        (18, 16, 1038, (414, 432, 450)),
    ):
        for index, x in enumerate(xs):
            spark = Image.new("RGBA", (width, height), (0, 0, 0, 0))
            draw = ImageDraw.Draw(spark)
            draw.arc((2, 2, width - 3, height - 3), 15 + index * 35, 290 + index * 25, fill=EMBER + (255,), width=2)
            draw.ellipse((width // 2 - 2, height // 2 - 2, width // 2 + 2, height // 2 + 2), fill=BRASS_LIGHT + (255,))
            atlas.paste(_flatten_key(spark), (x, y))

    # Summoning placement pulse.
    for index, x in enumerate((704, 784, 864, 944)):
        pulse = Image.new("RGBA", (80, 80), (0, 0, 0, 0))
        radius = 16 + index * 7
        _rune_ring(pulse, (40, 40), radius, CYAN if index < 3 else BRASS_LIGHT, phase=index * 0.55, width=2)
        atlas.paste(_flatten_key(pulse), (x, 1050))

    # Four Winds miniature town.
    town = Image.new("RGBA", (24, 50), (0, 0, 0, 0))
    draw = ImageDraw.Draw(town)
    draw.polygon(((3, 46), (21, 46), (19, 18), (15, 18), (13, 4), (10, 4), (8, 18), (5, 18)), fill=(28, 34, 35, 255), outline=BRASS_LIGHT + (255,))
    draw.line((12, 8, 12, 43), fill=CYAN + (255,), width=2)
    atlas.paste(_flatten_key(town), (984, 918))
    atlas.save(output)


def build_all(repo: Path) -> None:
    source = repo / "art" / "reborn" / "source" / "interface" / "remaining-ui" / "unknown-wanderer.png"
    destination = repo / "themes" / "reborn" / "assets" / "images"
    if not source.is_file():
        raise FileNotFoundError(f"Missing approved Unknown Wanderer source: {source}")
    destination.mkdir(parents=True, exist_ok=True)

    build_cast_informer(destination / "cast_informer_idle.png", "idle")
    build_cast_informer(destination / "cast_informer_pressed.png", "pressed")
    build_cast_informer(destination / "cast_informer.png", "ready")
    charbio = build_charbio9(source, destination / "charbio9.png")
    build_charselect(charbio, destination / "charselect.png")
    build_charselectoverlay(destination / "charselectoverlay.png")
    build_chatbox(destination / "chatbox.png")
    _effect_strip(4, destination / "combat_animation4.png")
    _effect_strip(5, destination / "combat_animation5.png")
    _effect_strip(6, destination / "combat_animation6.png")
    build_listspell(destination / "listspell.png")
    build_map_atlas(destination / "map.png")

    expected = {
        "cast_informer_idle.png": (32, 84),
        "cast_informer_pressed.png": (32, 84),
        "cast_informer.png": (32, 84),
        "charbio9.png": (264, 516),
        "charselect.png": (1024, 768),
        "charselectoverlay.png": (280, 140),
        "chatbox.png": (776, 236),
        "combat_animation4.png": (700, 140),
        "combat_animation5.png": (700, 140),
        "combat_animation6.png": (700, 140),
        "listspell.png": (768, 516),
        "map.png": (1024, 1159),
    }
    for name, size in expected.items():
        actual = Image.open(destination / name).size
        if actual != size:
            raise RuntimeError(f"{name}: expected {size}, got {actual}")
        print(f"Built {name}: {actual[0]}x{actual[1]}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", type=Path, default=_root())
    args = parser.parse_args()
    build_all(args.repo.resolve())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
