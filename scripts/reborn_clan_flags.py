#!/usr/bin/env python3
"""Build the compact clan standards used by the Reborn theme.

The selection-screen crests are intentionally elaborate.  At map scale they
would collapse into noise, so this builder keeps their colour and silhouette
while putting them on a shared metal-and-cloth standard.
"""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageEnhance, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "art/reborn/source/clans/components"
OUTPUT = ROOT / "themes/reborn/assets/images"

CLANS = {
    "red": ((112, 17, 24), (212, 53, 47)),
    "yellow": ((125, 87, 12), (225, 177, 47)),
    "aqua": ((7, 91, 106), (44, 185, 195)),
    "purple": ((72, 28, 91), (151, 67, 171)),
}


def emblem(name: str, size: tuple[int, int]) -> Image.Image:
    source = Image.open(SOURCE / f"{name}.png").convert("RGBA")
    bounds = source.getchannel("A").getbbox()
    if not bounds:
        raise RuntimeError(f"empty clan emblem: {name}")
    source = source.crop(bounds)
    source.thumbnail(size, Image.Resampling.LANCZOS)
    source = ImageEnhance.Contrast(source).enhance(1.15)
    source = ImageEnhance.Sharpness(source).enhance(1.8)
    return source


def gradient_cloth(size: tuple[int, int], dark: tuple[int, int, int], light: tuple[int, int, int]) -> Image.Image:
    width, height = size
    cloth = Image.new("RGBA", size)
    px = cloth.load()
    for y in range(height):
        for x in range(width):
            wave = (x * 19 + y * 7) % 11
            shine = max(0.0, 1.0 - abs((x / max(1, width - 1)) - 0.42) * 2.3)
            mix = 0.18 + shine * 0.46 + wave / 90.0
            px[x, y] = tuple(int(dark[i] * (1.0 - mix) + light[i] * mix) for i in range(3)) + (255,)
    return cloth


def standard(name: str, size: tuple[int, int], *, compact: bool = False, raised: bool = False) -> Image.Image:
    width, height = size
    scale = width / 35.0
    result = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(result)

    pole_x = max(2, round(4 * scale))
    top = max(1, round(2 * scale))
    bottom = height - max(1, round(2 * scale))
    metal_dark = (54, 57, 58, 255)
    metal = (164, 151, 119, 255)
    metal_light = (224, 207, 157, 255)
    draw.line((pole_x, top + 2, pole_x, bottom), fill=metal_dark, width=max(2, round(3 * scale)))
    draw.line((pole_x, top + 2, pole_x, bottom), fill=metal, width=max(1, round(1 * scale)))
    finial = max(2, round(3 * scale))
    draw.polygon(((pole_x, top), (pole_x - finial, top + finial + 1), (pole_x, top + finial),
                  (pole_x + finial, top + finial + 1)), fill=metal_light, outline=metal_dark)

    x0 = pole_x + max(1, round(1 * scale))
    x1 = width - max(1, round(2 * scale))
    y0 = top + max(3, round(4 * scale))
    y1 = round(height * (0.66 if compact else 0.70))
    if raised:
        y0 = max(top + 2, y0 - 1)
        y1 = max(y0 + 6, y1 - 2)

    # A shallow swallowtail remains legible at 22 pixels and distinguishes the
    # new standards from the original robe-shaped flags.
    notch = max(2, round((x1 - x0) * 0.18))
    mask = Image.new("L", size, 0)
    md = ImageDraw.Draw(mask)
    md.polygon(((x0, y0), (x1, y0 + 1), (x1, y1 - 2),
                (x1 - notch, y1 - 5), (x1 - notch * 2, y1), (x0, y1 - 1)), fill=255)
    cloth = gradient_cloth(size, *CLANS[name])
    cloth.putalpha(mask)
    result.alpha_composite(cloth)

    outline = Image.new("RGBA", size, (0, 0, 0, 0))
    od = ImageDraw.Draw(outline)
    edge = ((x0, y0), (x1, y0 + 1), (x1, y1 - 2),
            (x1 - notch, y1 - 5), (x1 - notch * 2, y1), (x0, y1 - 1))
    od.line(edge + (edge[0],), fill=metal_dark, width=max(1, round(scale)))
    od.line(((x0 + 1, y0 + 1), (x1 - 1, y0 + 2)), fill=metal_light, width=1)
    result.alpha_composite(outline)

    symbol_size = (max(8, x1 - x0 - 5), max(8, y1 - y0 - 5))
    symbol = emblem(name, symbol_size)
    # A restrained shadow separates silver details from yellow/aqua cloth.
    shadow = Image.new("RGBA", symbol.size, (0, 0, 0, 0))
    shadow.putalpha(symbol.getchannel("A").filter(ImageFilter.GaussianBlur(max(0.35, scale * 0.45))))
    shadow_color = Image.new("RGBA", symbol.size, (0, 0, 0, 150))
    shadow_color.putalpha(shadow.getchannel("A"))
    sx = x0 + (x1 - x0 - symbol.width) // 2
    sy = y0 + (y1 - y0 - symbol.height) // 2
    result.alpha_composite(shadow_color, (sx + 1, sy + 1))
    result.alpha_composite(symbol, (sx, sy))
    return result


def army_marker(name: str, *, raised: bool = False) -> Image.Image:
    """Build the animated map marker for a party stationed in a land.

    Map parties used to be represented by a tiny cloth flag.  On the darker
    Reborn map that made armies easy to confuse with towns and summoning
    circles.  The marker keeps the original 35x50 anchor box, but presents the
    approved clan crest as a metal map pin with a bright clan-colour rim.
    """
    size = (35, 50)
    result = Image.new("RGBA", size, (0, 0, 0, 0))
    dark, light = CLANS[name]
    lift = -2 if raised else 0

    # A restrained coloured halo survives downscaling and separates the pin
    # from both forests and the volcanic centre of the adventure map.
    halo = Image.new("RGBA", size, (0, 0, 0, 0))
    hd = ImageDraw.Draw(halo)
    hd.ellipse((2, 2 + lift, 32, 36 + lift), fill=light + (150 if raised else 105,))
    halo = halo.filter(ImageFilter.GaussianBlur(2.4 if raised else 1.8))
    result.alpha_composite(halo)

    draw = ImageDraw.Draw(result)
    outer = [
        (17, 1 + lift), (27, 4 + lift), (33, 12 + lift),
        (31, 29 + lift), (22, 37 + lift), (17, 48),
        (12, 37 + lift), (4, 29 + lift), (2, 12 + lift),
        (8, 4 + lift),
    ]
    draw.polygon(outer, fill=(35, 37, 38, 245), outline=(17, 18, 19, 255))

    rim = [
        (17, 4 + lift), (26, 7 + lift), (30, 13 + lift),
        (28, 27 + lift), (21, 33 + lift), (17, 42 + lift),
        (13, 33 + lift), (7, 27 + lift), (5, 13 + lift),
        (10, 7 + lift),
    ]
    draw.line(rim + [rim[0]], fill=(203, 184, 133, 255), width=2)

    plate = (7, 7 + lift, 29, 31 + lift)
    draw.ellipse(plate, fill=dark + (255,), outline=light + (255,), width=2)
    draw.arc((9, 9 + lift, 27, 29 + lift), 200, 340,
             fill=(235, 222, 181, 205), width=1)

    symbol = emblem(name, (22, 22))
    sx = (size[0] - symbol.width) // 2
    sy = 8 + lift + (22 - symbol.height) // 2
    shadow_alpha = symbol.getchannel("A").filter(ImageFilter.GaussianBlur(1.0))
    shadow = Image.new("RGBA", symbol.size, (0, 0, 0, 170))
    shadow.putalpha(shadow_alpha)
    result.alpha_composite(shadow, (sx + 1, sy + 1))
    result.alpha_composite(symbol, (sx, sy))

    # The second frame is a light pulse rather than a different silhouette,
    # so the marker never appears to jump to a neighbouring province.
    if raised:
        flash = Image.new("RGBA", size, (0, 0, 0, 0))
        fd = ImageDraw.Draw(flash)
        fd.ellipse((7, 7 + lift, 29, 31 + lift), outline=(255, 245, 207, 130), width=1)
        result = Image.alpha_composite(result, flash)
    return result


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    for name in CLANS:
        outputs = {
            f"clan_flag_{name}.png": standard(name, (35, 46)),
            f"clan_flag_{name}_small.png": standard(name, (22 if name == "red" else 24, 30), compact=True),
            f"clan_townflag_{name}.png": army_marker(name),
            f"clan_townflag_{name}_raised.png": army_marker(name, raised=True),
        }
        for filename, image in outputs.items():
            target = OUTPUT / filename
            image.save(target, optimize=True)
            print(target.relative_to(ROOT))


if __name__ == "__main__":
    main()
