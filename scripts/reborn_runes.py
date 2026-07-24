#!/usr/bin/env python3
"""Build the Reborn rune faces from a small, deterministic component sheet."""

from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "art/reborn/source/mahjong/runes/components/rune-components.png"
RUNES = ROOT / "art/reborn/source/mahjong/runes"
FONT = ROOT / "themes/reborn/assets/fonts/forum-regular.ttf"

SIZES = {
    "large": (56, 70),
    "medium": (40, 48),
    "small": (24, 30),
}

WIND_FILES = {
    1: "wind_blue_east.png",
    2: "wind_blue_south.png",
    3: "wind_blue_west.png",
    4: "wind_blue_north.png",
}


def sheet_cell(sheet: Image.Image, column: int, row: int) -> Image.Image:
    cell_w = sheet.width // 3
    cell_h = sheet.height // 3
    pad = 4
    cell = sheet.crop(
        (
            column * cell_w + pad,
            row * cell_h + pad,
            (column + 1) * cell_w - pad,
            (row + 1) * cell_h - pad,
        )
    )
    alpha = cell.getchannel("A")
    bounds = alpha.getbbox()
    return cell.crop(bounds) if bounds else cell


def contain(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    result = Image.new("RGBA", size)
    copy = image.copy()
    copy.thumbnail(size, Image.Resampling.LANCZOS)
    result.alpha_composite(copy, ((size[0] - copy.width) // 2, (size[1] - copy.height) // 2))
    return result


def remove_legacy_color_key(image: Image.Image) -> Image.Image:
    """Convert the old Rune War pink key into real alpha for composition."""
    result = image.convert("RGBA")
    pixels = result.load()
    key = (255, 210, 210)
    for y in range(result.height):
        for x in range(result.width):
            red, green, blue, alpha = pixels[x, y]
            distance = abs(red - key[0]) + abs(green - key[1]) + abs(blue - key[2])
            if distance <= 18:
                pixels[x, y] = (red, green, blue, 0)
            elif distance <= 72:
                pixels[x, y] = (red, green, blue, round(alpha * (distance - 18) / 54))
    return result


def tile_base(component: Image.Image, size: tuple[int, int]) -> Image.Image:
    width, height = size
    # The generated source is square. A controlled vertical stretch turns it
    # into the established Rune War stone silhouette without changing runtime
    # sprite dimensions or atlas coordinates.
    base = component.resize((width, height), Image.Resampling.LANCZOS)
    background = Image.new("RGBA", size, (24, 28, 29, 255))
    background.alpha_composite(base)
    return background.convert("RGB").convert("RGBA")


def pip_layout(rank: int) -> list[tuple[float, float]]:
    layouts = {
        1: [(0.50, 0.50)],
        2: [(0.31, 0.27), (0.69, 0.73)],
        3: [(0.30, 0.24), (0.50, 0.50), (0.70, 0.76)],
        4: [(0.30, 0.27), (0.70, 0.27), (0.30, 0.73), (0.70, 0.73)],
        5: [(0.29, 0.25), (0.71, 0.25), (0.50, 0.50), (0.29, 0.75), (0.71, 0.75)],
        6: [(0.30, 0.22), (0.70, 0.22), (0.30, 0.50), (0.70, 0.50), (0.30, 0.78), (0.70, 0.78)],
        7: [(0.29, 0.20), (0.71, 0.20), (0.29, 0.50), (0.50, 0.50), (0.71, 0.50), (0.29, 0.80), (0.71, 0.80)],
        8: [(0.29, 0.17), (0.71, 0.17), (0.29, 0.39), (0.71, 0.39), (0.29, 0.61), (0.71, 0.61), (0.29, 0.83), (0.71, 0.83)],
        9: [(0.27, 0.18), (0.50, 0.18), (0.73, 0.18), (0.27, 0.50), (0.50, 0.50), (0.73, 0.50), (0.27, 0.82), (0.50, 0.82), (0.73, 0.82)],
    }
    return layouts[rank]


def add_pips(base: Image.Image, icon: Image.Image, rank: int) -> Image.Image:
    result = base.copy()
    width, height = result.size
    if rank == 1:
        max_size = (round(width * 0.42), round(height * 0.55))
    elif rank <= 5:
        max_size = (round(width * 0.25), round(height * 0.29))
    elif rank <= 8:
        max_size = (round(width * 0.22), round(height * 0.22))
    else:
        max_size = (round(width * 0.19), round(height * 0.20))
    pip = contain(icon, max_size)
    for x_ratio, y_ratio in pip_layout(rank):
        x = round(width * x_ratio - pip.width / 2)
        y = round(height * y_ratio - pip.height / 2)
        result.alpha_composite(pip, (x, y))
    return result


def add_water_number(base: Image.Image, rank: int) -> Image.Image:
    width, height = base.size
    layer = Image.new("RGBA", base.size)
    font_size = max(12, round(height * 0.60))
    font = ImageFont.truetype(str(FONT), font_size)
    text = str(rank)
    draw = ImageDraw.Draw(layer)
    bounds = draw.textbbox((0, 0), text, font=font, stroke_width=1)
    x = (width - (bounds[2] - bounds[0])) // 2 - bounds[0]
    y = (height - (bounds[3] - bounds[1])) // 2 - bounds[1]
    glow = Image.new("RGBA", base.size)
    glow_draw = ImageDraw.Draw(glow)
    glow_draw.text((x, y), text, font=font, fill=(16, 122, 224, 230), stroke_width=max(1, height // 24), stroke_fill=(12, 74, 155, 245))
    glow = glow.filter(ImageFilter.GaussianBlur(max(0.5, height / 45)))
    result = base.copy()
    result.alpha_composite(glow)
    draw.text((x, y), text, font=font, fill=(41, 161, 236, 255), stroke_width=max(1, height // 35), stroke_fill=(4, 65, 133, 255))
    # A narrow highlight makes the numeral look carved from moving water while
    # keeping every rank unmistakable at the 24x30 runtime size.
    draw.text((x, y - max(1, height // 35)), text, font=font, fill=(143, 231, 255, 205))
    result.alpha_composite(layer)
    return result


def alpha_centroid_x(image: Image.Image) -> float:
    alpha = image.getchannel("A")
    total = 0
    weighted_x = 0
    for y in range(alpha.height):
        for x in range(alpha.width):
            value = alpha.getpixel((x, y))
            total += value
            weighted_x += x * value
    return weighted_x / total if total else image.width / 2


def add_centered(
    base: Image.Image,
    icon: Image.Image,
    scale: tuple[float, float],
    *,
    center_visual_mass: bool = False,
) -> Image.Image:
    width, height = base.size
    fitted = contain(icon, (round(width * scale[0]), round(height * scale[1])))
    result = base.copy()
    x = (width - fitted.width) // 2
    if center_visual_mass:
        x += round(fitted.width / 2 - alpha_centroid_x(fitted))
    result.alpha_composite(fitted, (x, (height - fitted.height) // 2))
    return result


def save_face(image: Image.Image, tier: str, family: int, rank: int) -> None:
    destination = RUNES / tier / f"stone_{tier}{family}{rank}.png"
    destination.parent.mkdir(parents=True, exist_ok=True)
    image.convert("RGB").save(destination, optimize=True)


def main() -> None:
    sheet = Image.open(SOURCE).convert("RGBA")
    empty_tile = sheet_cell(sheet, 0, 0)
    sword = sheet_cell(sheet, 1, 0)
    skull = sheet_cell(sheet, 2, 0)
    red_dragon = sheet_cell(sheet, 1, 1)
    green_dragon = sheet_cell(sheet, 2, 1)
    silver_dragon = sheet_cell(sheet, 0, 2)
    tile_back = sheet_cell(sheet, 1, 2)
    dragons = {1: red_dragon, 2: green_dragon, 3: silver_dragon}

    for tier, size in SIZES.items():
        base = tile_base(empty_tile, size)
        back = tile_base(tile_back, size)
        save_face(back, tier, 0, 0)

        for rank in range(1, 10):
            save_face(add_pips(base, skull, rank), tier, 1, rank)
            save_face(add_pips(base, sword, rank), tier, 2, rank)
            save_face(add_water_number(base, rank), tier, 3, rank)

        for rank, filename in WIND_FILES.items():
            wind_path = ROOT / "art/reborn/source/mahjong/markers/winds" / filename
            wind = remove_legacy_color_key(Image.open(wind_path))
            save_face(add_centered(base, wind, (0.60, 0.62)), tier, 4, rank)

        for rank, dragon in dragons.items():
            save_face(
                add_centered(base, dragon, (0.70, 0.66), center_visual_mass=True),
                tier,
                5,
                rank,
            )

    print("Built 105 Reborn rune faces from shared components.")


if __name__ == "__main__":
    main()
