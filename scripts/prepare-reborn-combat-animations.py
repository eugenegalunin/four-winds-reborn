#!/usr/bin/env python3
"""Build Reborn-only Mahjong guardians and combat effect atlases.

The runtime still consumes fixed legacy sprite-sheet geometry.  This builder
keeps those dimensions and the ``#ffd2d2`` chroma-key contract while deriving
every frame from one approved source illustration per guardian.  That avoids
the identity drift produced by generating animation frames independently.
"""

from __future__ import annotations

import argparse
import math
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageOps


CHROMA = (255, 210, 210, 255)
EDGE_MATTE = (4, 8, 10, 255)


def crop_alpha(image: Image.Image) -> Image.Image:
    rgba = image.convert("RGBA")
    box = rgba.getchannel("A").getbbox()
    if box is None:
        raise ValueError("Source cutout is fully transparent")
    return rgba.crop(box)


def contain(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    scale = min(size[0] / image.width, size[1] / image.height)
    return image.resize(
        (max(1, round(image.width * scale)), max(1, round(image.height * scale))),
        Image.Resampling.LANCZOS,
    )


def tint_alpha(image: Image.Image, color: tuple[int, int, int], alpha: int) -> Image.Image:
    glow = Image.new("RGBA", image.size, color + (0,))
    mask = image.getchannel("A").filter(ImageFilter.GaussianBlur(7))
    mask = mask.point(lambda value: value * alpha // 255)
    glow.putalpha(mask)
    return glow


def flatten_to_colorkey(image: Image.Image, threshold: int = 5) -> Image.Image:
    """Flatten RGBA art without baking pink into antialiased edges.

    The engine removes only the exact legacy key colour.  Compositing soft
    edges straight onto pink would therefore leave a bright fringe.  We bake
    visible edge pixels against the arena's near-black tone and retain exact
    pink only outside the binary silhouette.
    """
    rgba = image.convert("RGBA")
    matte = Image.new("RGBA", rgba.size, EDGE_MATTE)
    visible = Image.alpha_composite(matte, rgba).convert("RGB")
    mask = rgba.getchannel("A").point(lambda value: 255 if value > threshold else 0)
    keyed = Image.new("RGB", rgba.size, CHROMA[:3])
    keyed.paste(visible, (0, 0), mask)
    return keyed


def masked_rotate(
    image: Image.Image,
    polygon: tuple[tuple[float, float], ...],
    pivot: tuple[float, float],
    angle: float,
) -> Image.Image:
    """Rotate one articulated part while keeping the rest of the cutout still."""
    if abs(angle) < 0.05:
        return image
    width, height = image.size
    mask = Image.new("L", image.size, 0)
    ImageDraw.Draw(mask).polygon(
        [(round(x * width), round(y * height)) for x, y in polygon],
        fill=255,
    )
    mask = mask.filter(ImageFilter.GaussianBlur(max(2, round(width / 320))))
    part = Image.new("RGBA", image.size, (0, 0, 0, 0))
    part.paste(image, (0, 0), ImageChops.multiply(image.getchannel("A"), mask))

    base = image.copy()
    base.putalpha(ImageChops.multiply(image.getchannel("A"), ImageChops.invert(mask)))
    moved = part.rotate(
        angle,
        resample=Image.Resampling.BICUBIC,
        center=(round(pivot[0] * width), round(pivot[1] * height)),
    )
    return Image.alpha_composite(base, moved)


def guardian_pose(source: Image.Image, action: str, level: float) -> Image.Image:
    """Articulate each guardian according to its actual declared action."""
    padded = ImageOps.expand(source, border=round(max(source.size) * 0.10), fill=(0, 0, 0, 0))

    if action == "hydra_heads":
        # Each neck sways around a different shoulder so Chao reads as three
        # living heads, not a single sprite bobbing up and down.
        posed = masked_rotate(
            padded,
            ((0.04, 0.24), (0.46, 0.23), (0.50, 0.66), (0.29, 0.76), (0.03, 0.67)),
            (0.36, 0.65),
            -14.0 * level,
        )
        posed = masked_rotate(
            posed,
            ((0.27, 0.02), (0.72, 0.02), (0.75, 0.58), (0.50, 0.69), (0.31, 0.43)),
            (0.51, 0.64),
            10.0 * level,
        )
        return masked_rotate(
            posed,
            ((0.58, 0.25), (0.97, 0.25), (0.98, 0.70), (0.73, 0.78), (0.57, 0.57)),
            (0.68, 0.67),
            15.0 * level,
        )

    if action == "raise_arm":
        # Raise the forward (viewer-left) fist from its shoulder.
        return masked_rotate(
            padded,
            ((0.01, 0.42), (0.46, 0.39), (0.50, 0.72), (0.29, 0.98), (0.01, 0.96)),
            (0.43, 0.48),
            34.0 * level,
        )

    if action in ("spin_ccw", "spin_lightning"):
        # Kong is a circular wraith by design: a full counter-clockwise turn
        # remains stable in its frame and is unmistakable at game scale.
        return padded.rotate(
            360.0 * level,
            resample=Image.Resampling.BICUBIC,
            center=(padded.width // 2, padded.height // 2),
        )

    if action == "petals":
        # Open the armoured flower around the still, glowing core.
        posed = masked_rotate(
            padded,
            ((0.02, 0.12), (0.50, 0.08), (0.51, 0.92), (0.02, 0.95)),
            (0.49, 0.68),
            18.0 * level,
        )
        return masked_rotate(
            posed,
            ((0.50, 0.08), (0.98, 0.12), (0.98, 0.95), (0.49, 0.92)),
            (0.51, 0.68),
            -18.0 * level,
        )

    return padded


def guardian_frame(
    source: Image.Image,
    frame_size: tuple[int, int],
    progress: float,
    *,
    fill: float,
    base_scale: float,
    pulse: float,
    angle: float,
    shift_x: float,
    glow: tuple[int, int, int],
    action: str,
    cyclic: bool,
) -> Image.Image:
    """Render one stable guardian pose plus an obvious action sequence."""
    width, height = frame_size
    eased = 0.5 - 0.5 * math.cos(progress * math.pi)
    action_level = math.sin(progress * math.pi) if cyclic else eased
    scale = base_scale + (1.0 - base_scale) * action_level
    scale *= 1.0 + pulse * math.sin(progress * math.pi * 2.0)

    posed_source = guardian_pose(source, action, action_level)
    if action not in ("spin_ccw", "spin_lightning"):
        posed_source = crop_alpha(posed_source)
    max_size = (round(width * fill * scale), round(height * fill * scale))
    sprite = contain(posed_source, max_size)
    if angle:
        sprite = sprite.rotate(
            angle * action_level,
            resample=Image.Resampling.BICUBIC,
            expand=True,
        )

    frame = Image.new("RGBA", frame_size, (0, 0, 0, 0))
    x = (width - sprite.width) // 2 + round(shift_x * action_level)
    y = height - sprite.height - round(height * 0.035)

    aura = tint_alpha(sprite, glow, round(80 + 85 * eased))
    frame.alpha_composite(aura, (x, y))
    frame.alpha_composite(sprite, (x, y))

    action_layer = guardian_action(frame_size, action, action_level, glow)
    frame = Image.alpha_composite(frame, action_layer)

    return flatten_to_colorkey(frame)


def action_line(
    image: Image.Image,
    points: list[tuple[float, float]],
    color: tuple[int, int, int],
    width: int,
) -> None:
    """Draw a compact coloured glow without the old opaque black fringe."""
    draw = ImageDraw.Draw(image)
    draw.line(points, fill=color + (105,), width=width * 3, joint="curve")
    draw.line(points, fill=color + (245,), width=width, joint="curve")
    draw.line(
        points,
        fill=(244, 250, 238, 235),
        width=max(1, width // 2),
        joint="curve",
    )


def guardian_action(
    frame_size: tuple[int, int],
    action: str,
    level: float,
    glow: tuple[int, int, int],
) -> Image.Image:
    """Create a readable declaration effect directed toward the table centre."""
    width, height = frame_size
    layer = Image.new("RGBA", frame_size, (0, 0, 0, 0))
    if level <= 0.025:
        return layer
    draw = ImageDraw.Draw(layer)
    alpha = round(225 * min(1.0, level * 1.35))

    if action == "breath":
        # Three converging emerald streams make all hydra heads participate.
        starts = ((0.50, 0.34), (0.61, 0.40), (0.56, 0.49))
        end = (0.98, 0.67)
        for index, start in enumerate(starts):
            ex = end[0] * width
            ey = end[1] * height + (index - 1) * 5
            sx = start[0] * width
            sy = start[1] * height
            mid = ((sx + ex) * 0.5, (sy + ey) * 0.5 - 10 * level)
            action_line(layer, [(sx, sy), mid, (ex, ey)], glow, 2 + index % 2)
    elif action == "impact":
        # A left-facing shock cone reads as the juggernaut's heavy impact.
        cx, cy = width * 0.26, height * 0.60
        radius = 12 + width * 0.42 * level
        for inset in (0, 10, 20):
            box = (cx - radius - inset, cy - radius, cx + radius + inset, cy + radius)
            draw.arc(box, 112, 248, fill=glow + (max(0, alpha - inset * 5),), width=4)
        for index in range(7):
            theta = math.radians(130 + index * 15)
            r0 = 18 + 25 * level
            r1 = r0 + 18 * level
            draw.line(
                (cx + math.cos(theta) * r0, cy + math.sin(theta) * r0,
                 cx + math.cos(theta) * r1, cy + math.sin(theta) * r1),
                fill=(245, 150, 63, alpha), width=2,
            )
    elif action in ("lightning", "spin_lightning"):
        # Kong throws a branching blue bolt up and inward.
        start = (width * 0.38, height * 0.43)
        end = (width * 0.03, height * 0.05)
        points = [start]
        for index in range(1, 6):
            t = index / 6
            points.append((
                start[0] + (end[0] - start[0]) * t + math.sin(index * 4.1) * 9 * level,
                start[1] + (end[1] - start[1]) * t,
            ))
        points.append(end)
        action_line(layer, points, glow, 4)
        for branch in (2, 4):
            bx, by = points[branch]
            action_line(layer, [(bx, by), (bx - 19 * level, by + 5), (bx - 30 * level, by - 8)], glow, 2)
    elif action == "oracle":
        # Game gathers and releases a complete violet orb pulse.
        cx, cy = width * 0.64, height * 0.35
        radius = 8 + 34 * level
        draw.ellipse(
            (cx - radius, cy - radius, cx + radius, cy + radius),
            outline=glow + (alpha,), width=4,
        )
        for index in range(6):
            theta = index * math.tau / 6 + level * 1.2
            r0 = radius + 7
            r1 = r0 + 12 * level
            draw.line(
                (cx + math.cos(theta) * r0, cy + math.sin(theta) * r0,
                 cx + math.cos(theta) * r1, cy + math.sin(theta) * r1),
                fill=(215, 157, 255, alpha), width=2,
            )

    return layer


def build_guardian_atlas(
    source_path: Path,
    output_path: Path,
    *,
    frame_size: tuple[int, int],
    columns: int,
    rows: int,
    fill: float,
    base_scale: float,
    pulse: float,
    angle: float,
    shift_x: float,
    glow: tuple[int, int, int],
    action: str,
    cyclic: bool = False,
) -> None:
    source = crop_alpha(Image.open(source_path))
    count = columns * rows
    atlas = Image.new("RGB", (frame_size[0] * columns, frame_size[1] * rows), CHROMA[:3])
    for index in range(count):
        progress = index / max(1, count - 1)
        frame = guardian_frame(
            source,
            frame_size,
            progress,
            fill=fill,
            base_scale=base_scale,
            pulse=pulse,
            angle=angle,
            shift_x=shift_x,
            glow=glow,
            action=action,
            cyclic=cyclic,
        )
        atlas.paste(frame, ((index % columns) * frame_size[0], (index // columns) * frame_size[1]))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(output_path, optimize=True)


def glow_line(
    image: Image.Image,
    points: list[tuple[float, float]],
    color: tuple[int, int, int],
    width: int,
) -> None:
    glow = Image.new("RGBA", image.size, (0, 0, 0, 0))
    gd = ImageDraw.Draw(glow)
    gd.line(points, fill=color + (165,), width=width * 4, joint="curve")
    glow = glow.filter(ImageFilter.GaussianBlur(width * 2.2))
    image.alpha_composite(glow)
    draw = ImageDraw.Draw(image)
    draw.line(points, fill=color + (245,), width=width, joint="curve")
    draw.line(points, fill=(255, 239, 193, 235), width=max(1, width // 3), joint="curve")


def melee_effect(progress: float, size: int = 140) -> Image.Image:
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    eased = 1.0 - (1.0 - progress) ** 2
    length = 25 + 78 * eased
    alpha = round(255 * math.sin(progress * math.pi * 0.88))
    cx, cy = size / 2, size / 2
    for slope, shift in ((-0.72, -9), (0.72, 9)):
        dx = length / 2
        dy = slope * dx
        points = [(cx - dx, cy - dy + shift), (cx + dx, cy + dy + shift)]
        glow_line(layer, points, (206, 55, 29), 7)
    sparks = ImageDraw.Draw(layer)
    for index in range(8):
        theta = index * math.tau / 8 + progress
        r0 = 18 + 34 * eased
        r1 = r0 + 7 + index % 3
        sparks.line(
            (
                cx + math.cos(theta) * r0,
                cy + math.sin(theta) * r0,
                cx + math.cos(theta) * r1,
                cy + math.sin(theta) * r1,
            ),
            fill=(239, 151, 64, alpha),
            width=2,
        )
    layer.putalpha(layer.getchannel("A").point(lambda value: value * alpha // 255))
    return layer


def missile_effect(progress: float, size: int = 140) -> Image.Image:
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)
    cx, cy = size // 2, size // 2
    radius = round(52 - 29 * progress)
    alpha = round(235 * math.sin((0.15 + 0.85 * progress) * math.pi))
    for offset, color in ((0, (79, 201, 224)), (8, (155, 113, 221))):
        box = (cx - radius - offset, cy - radius - offset, cx + radius + offset, cy + radius + offset)
        draw.arc(box, 18 + 120 * progress, 152 + 120 * progress, fill=color + (alpha,), width=3)
        draw.arc(box, 198 + 120 * progress, 332 + 120 * progress, fill=color + (alpha,), width=3)
    bolt_x = round(14 + 82 * progress)
    glow_line(layer, [(bolt_x - 27, cy + 14), (bolt_x, cy), (bolt_x + 29, cy - 15)], (58, 174, 220), 5)
    return layer


def shield_effect(progress: float, size: int = 140) -> Image.Image:
    layer = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(layer)
    cx, cy = size // 2, size // 2
    radius = round(20 + 42 * math.sin(progress * math.pi / 2))
    alpha = round(235 * math.sin((0.12 + 0.88 * progress) * math.pi))
    draw.ellipse((cx - radius, cy - radius, cx + radius, cy + radius), fill=(88, 31, 10, alpha // 4), outline=(225, 115, 37, alpha), width=5)
    for index in range(6):
        theta = index * math.tau / 6 + progress * 0.55
        x = cx + math.cos(theta) * radius
        y = cy + math.sin(theta) * radius
        rr = 7
        draw.regular_polygon((x, y, rr), 4, rotation=45 + math.degrees(theta), fill=(238, 151, 64, alpha), outline=(255, 224, 155, alpha))
    glow = layer.filter(ImageFilter.GaussianBlur(7))
    glow.putalpha(glow.getchannel("A").point(lambda value: value * 3 // 5))
    return Image.alpha_composite(glow, layer)


def build_effect_atlas(output_path: Path, factory) -> None:
    size = 140
    atlas = Image.new("RGB", (size * 5, size), CHROMA[:3])
    for index in range(5):
        effect = factory(index / 4.0, size)
        atlas.paste(flatten_to_colorkey(effect, threshold=2), (index * size, 0))
    output_path.parent.mkdir(parents=True, exist_ok=True)
    atlas.save(output_path, optimize=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-dir", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    args = parser.parse_args()

    sources = args.source_dir
    outputs = args.output_dir
    build_guardian_atlas(
        sources / "guardian-chao-hydra.png",
        outputs / "anihydra.png",
        frame_size=(250, 250), columns=4, rows=2,
        fill=0.72, base_scale=0.96, pulse=0.0, angle=0.0, shift_x=0.0,
        glow=(61, 220, 161), action="breath",
    )
    build_guardian_atlas(
        sources / "guardian-pung-juggernaut.png",
        outputs / "anijuger.png",
        frame_size=(204, 200), columns=5, rows=1,
        fill=0.80, base_scale=0.97, pulse=0.0, angle=0.0, shift_x=-10.0,
        glow=(238, 91, 38), action="impact",
    )
    build_guardian_atlas(
        sources / "guardian-kong-wraith.png",
        outputs / "anidemon.png",
        frame_size=(300, 300), columns=3, rows=3,
        fill=0.66, base_scale=0.98, pulse=0.0, angle=0.0, shift_x=0.0,
        glow=(74, 190, 255), action="spin_lightning",
    )
    build_guardian_atlas(
        sources / "guardian-game-oracle.png",
        outputs / "aniwizar.png",
        frame_size=(250, 250), columns=4, rows=5,
        fill=0.72, base_scale=0.98, pulse=0.0, angle=0.0, shift_x=0.0,
        glow=(175, 86, 255), action="oracle", cyclic=True,
    )

    build_effect_atlas(outputs / "combat_animation1.png", melee_effect)
    build_effect_atlas(outputs / "combat_animation2.png", missile_effect)
    build_effect_atlas(outputs / "combat_animation3.png", shield_effect)


if __name__ == "__main__":
    main()
