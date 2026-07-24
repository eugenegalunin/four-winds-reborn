#!/usr/bin/env python3
"""Extract, compose, rebuild, and verify the Reborn Mahjong atlas.

The runtime still consumes the legacy all.png layout. This tool turns that
layout into semantic source assets so artists can replace one group at a time,
then deterministically compiles those sources back to the compatible atlas.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any

from PIL import Image, ImageChops


MANIFEST_NAME = "atlas-manifest.json"
BASE_NAME = "atlas-base.png"
EXPECTED_SIZE = (1024, 1296)
COLORKEY = (255, 210, 210)
BOARD_SIZE = (1024, 768)


def load_json(path: Path) -> Any:
    with path.open("r", encoding="utf-8-sig") as stream:
        return json.load(stream)


def save_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, ensure_ascii=False, indent=2)
        stream.write("\n")


def category_for(sprite_id: str) -> str:
    if sprite_id == "screen3":
        return "board"
    if sprite_id.startswith("stone_large"):
        return "runes/large"
    if sprite_id.startswith("stone_medium"):
        return "runes/medium"
    if sprite_id.startswith("stone_small"):
        return "runes/small"
    if sprite_id.startswith(("stone_active", "stone_selected")):
        return "markers/selection"
    if sprite_id.startswith("wind"):
        return "markers/winds"
    if sprite_id.startswith("turn_delay"):
        return "markers/turn-delay"
    if sprite_id.startswith("luck"):
        return "markers/luck"
    if sprite_id.startswith("affected"):
        return "markers/effects"
    return "misc"


def atlas_entries(images_json: Path) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    for item in load_json(images_json):
        if item.get("file") != "all.png" or "crop" not in item:
            continue
        crop = item["crop"]
        if len(crop) != 4 or any(not isinstance(value, int) for value in crop):
            raise ValueError(f"Invalid crop for {item.get('id')}: {crop!r}")
        entries.append({"id": item["id"], "crop": crop})
    if not entries:
        raise ValueError(f"No all.png entries found in {images_json}")
    return entries


def palette_index(image: Image.Image, rgb: tuple[int, int, int]) -> int:
    if image.mode != "P":
        raise ValueError(f"Expected indexed PNG, got mode {image.mode}")
    palette = image.getpalette()
    if palette is None:
        raise ValueError("Indexed PNG has no palette")
    for index in range(256):
        if tuple(palette[index * 3 : index * 3 + 3]) == rgb:
            return index
    raise ValueError(f"Palette does not contain colorkey {rgb}")


def ensure_bounds(entries: list[dict[str, Any]], size: tuple[int, int]) -> None:
    width, height = size
    for entry in entries:
        x, y, w, h = entry["crop"]
        if w <= 0 or h <= 0 or x < 0 or y < 0 or x + w > width or y + h > height:
            raise ValueError(f"Crop outside atlas for {entry['id']}: {entry['crop']}")


def extract(args: argparse.Namespace) -> None:
    atlas_path = args.atlas.resolve()
    output_dir = args.output.resolve()
    atlas = Image.open(atlas_path)
    atlas.load()
    if atlas.size != EXPECTED_SIZE:
        raise ValueError(f"Expected atlas size {EXPECTED_SIZE}, got {atlas.size}")

    entries = atlas_entries(args.images_json)
    ensure_bounds(entries, atlas.size)
    key_index = palette_index(atlas, COLORKEY)

    base = atlas.copy()
    manifest_entries: list[dict[str, Any]] = []
    for entry in entries:
        sprite_id = entry["id"]
        x, y, w, h = entry["crop"]
        relative = Path(category_for(sprite_id)) / f"{sprite_id}.png"
        target = output_dir / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        atlas.crop((x, y, x + w, y + h)).save(target, optimize=False)
        base.paste(key_index, (x, y, x + w, y + h))
        manifest_entries.append(
            {"id": sprite_id, "file": relative.as_posix(), "crop": [x, y, w, h]}
        )

    base.save(output_dir / BASE_NAME, optimize=False)
    save_json(
        output_dir / MANIFEST_NAME,
        {
            "version": 1,
            "canvas": list(atlas.size),
            "mode": atlas.mode,
            "colorkey": "#ffd2d2",
            "base": BASE_NAME,
            "entries": manifest_entries,
        },
    )
    print(f"Extracted {len(entries)} sprites to {output_dir}")


def load_manifest(source_dir: Path) -> dict[str, Any]:
    manifest = load_json(source_dir / MANIFEST_NAME)
    if manifest.get("version") != 1:
        raise ValueError(f"Unsupported manifest version: {manifest.get('version')}")
    if tuple(manifest.get("canvas", ())) != EXPECTED_SIZE:
        raise ValueError(f"Unexpected manifest canvas: {manifest.get('canvas')}")
    if manifest.get("colorkey", "").lower() != "#ffd2d2":
        raise ValueError(f"Unexpected manifest colorkey: {manifest.get('colorkey')}")
    return manifest


def build_atlas(source_dir: Path, output_path: Path) -> None:
    manifest = load_manifest(source_dir)
    base_path = source_dir / manifest["base"]
    canvas = Image.open(base_path)
    canvas.load()
    if canvas.size != EXPECTED_SIZE or canvas.mode != manifest["mode"]:
        raise ValueError(f"Base canvas mismatch: size={canvas.size}, mode={canvas.mode}")

    entries = manifest["entries"]
    ensure_bounds(entries, canvas.size)
    for entry in entries:
        x, y, w, h = entry["crop"]
        sprite_path = source_dir / entry["file"]
        sprite = Image.open(sprite_path)
        sprite.load()
        if sprite.size != (w, h):
            raise ValueError(
                f"Sprite size mismatch for {entry['id']}: expected {(w, h)}, got {sprite.size}"
            )
        if canvas.mode == "P":
            same_palette = sprite.mode == "P" and sprite.getpalette() == canvas.getpalette()
            if not same_palette:
                sprite = sprite.convert("RGB").quantize(
                    palette=canvas,
                    dither=Image.Dither.FLOYDSTEINBERG,
                )
        elif sprite.mode != canvas.mode:
            sprite = sprite.convert(canvas.mode)
        canvas.paste(sprite, (x, y))

    output_path.parent.mkdir(parents=True, exist_ok=True)
    canvas.save(output_path, optimize=False)
    print(f"Built {output_path} from {len(entries)} sprites")


def compose_board(args: argparse.Namespace) -> None:
    manifest_path = args.manifest.resolve()
    output_path = args.output.resolve()
    manifest = load_json(manifest_path)
    if manifest.get("version") != 1:
        raise ValueError(f"Unsupported board manifest version: {manifest.get('version')}")
    if tuple(manifest.get("canvas", ())) != BOARD_SIZE:
        raise ValueError(f"Unexpected board canvas: {manifest.get('canvas')}")

    source_dir = manifest_path.parent
    background = manifest["background"]
    if isinstance(background, str):
        background = {"file": background}
    background_path = source_dir / background["file"]
    board = Image.open(background_path).convert("RGBA")
    target_size = tuple(background.get("size", BOARD_SIZE))
    if target_size != BOARD_SIZE:
        raise ValueError(f"Board background target must be {BOARD_SIZE}, got {target_size}")
    if board.size != target_size:
        board = board.resize(target_size, Image.Resampling.LANCZOS)

    enabled_layers = [layer for layer in manifest.get("layers", []) if layer.get("enabled", True)]
    for layer in enabled_layers:
        layer_path = source_dir / layer["file"]
        image = Image.open(layer_path).convert("RGBA")
        source_crop = layer.get("sourceCrop")
        if source_crop is not None:
            if len(source_crop) != 4:
                raise ValueError(
                    f"Invalid source crop for board layer {layer['id']}: {source_crop}"
                )
            crop_x, crop_y, crop_width, crop_height = source_crop
            if (
                crop_x < 0
                or crop_y < 0
                or crop_width <= 0
                or crop_height <= 0
                or crop_x + crop_width > image.width
                or crop_y + crop_height > image.height
            ):
                raise ValueError(
                    f"Source crop outside image for board layer {layer['id']}: "
                    f"{source_crop} in {image.size}"
                )
            image = image.crop(
                (crop_x, crop_y, crop_x + crop_width, crop_y + crop_height)
            )
        expected_size = tuple(layer.get("size", image.size))
        if image.size != expected_size:
            image = image.resize(expected_size, Image.Resampling.LANCZOS)
        position = tuple(layer.get("position", (0, 0)))
        if len(position) != 2:
            raise ValueError(f"Invalid position for board layer {layer['id']}: {position}")
        x, y = position
        if x < 0 or y < 0 or x + image.width > board.width or y + image.height > board.height:
            raise ValueError(f"Board layer outside canvas: {layer['id']} at {position}")
        board.alpha_composite(image, position)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    board.convert("RGB").save(output_path, optimize=False)
    print(f"Composed {output_path} from {len(enabled_layers)} layers")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def verify(args: argparse.Namespace) -> None:
    source_dir = args.source.resolve()
    output_path = args.output.resolve()
    reference_path = args.reference.resolve()
    build_atlas(source_dir, output_path)

    output = Image.open(output_path).convert("RGBA")
    reference = Image.open(reference_path).convert("RGBA")
    if output.size != reference.size:
        raise ValueError(f"Image size mismatch: {output.size} != {reference.size}")
    difference = ImageChops.difference(output, reference)
    if difference.getbbox() is not None:
        changed = sum(1 for pixel in difference.getdata() if pixel != (0, 0, 0, 0))
        raise ValueError(f"Round-trip changed {changed} decoded pixels")

    manifest = load_manifest(source_dir)
    json_entries = atlas_entries(args.images_json)
    compact_manifest = [(entry["id"], entry["crop"]) for entry in manifest["entries"]]
    compact_json = [(entry["id"], entry["crop"]) for entry in json_entries]
    if compact_manifest != compact_json:
        raise ValueError("Manifest entries do not match images.json")

    print("Round-trip verification passed: 0 changed decoded pixels")
    print(f"Reference SHA-256: {sha256(reference_path)}")
    print(f"Output SHA-256:    {sha256(output_path)}")


def parser() -> argparse.ArgumentParser:
    result = argparse.ArgumentParser(description=__doc__)
    commands = result.add_subparsers(dest="command", required=True)

    extract_parser = commands.add_parser("extract", help="Split all.png into semantic sources")
    extract_parser.add_argument("--atlas", type=Path, required=True)
    extract_parser.add_argument("--images-json", type=Path, required=True)
    extract_parser.add_argument("--output", type=Path, required=True)
    extract_parser.set_defaults(handler=extract)

    build_parser = commands.add_parser("build", help="Compile semantic sources into all.png")
    build_parser.add_argument("--source", type=Path, required=True)
    build_parser.add_argument("--output", type=Path, required=True)
    build_parser.set_defaults(handler=lambda args: build_atlas(args.source.resolve(), args.output.resolve()))

    board_parser = commands.add_parser(
        "compose-board", help="Compose the 1024x768 Rune Game board from semantic layers"
    )
    board_parser.add_argument("--manifest", type=Path, required=True)
    board_parser.add_argument("--output", type=Path, required=True)
    board_parser.set_defaults(handler=compose_board)

    verify_parser = commands.add_parser("verify", help="Build and compare with a reference atlas")
    verify_parser.add_argument("--source", type=Path, required=True)
    verify_parser.add_argument("--output", type=Path, required=True)
    verify_parser.add_argument("--reference", type=Path, required=True)
    verify_parser.add_argument("--images-json", type=Path, required=True)
    verify_parser.set_defaults(handler=verify)
    return result


def main() -> None:
    args = parser().parse_args()
    args.handler(args)


if __name__ == "__main__":
    main()
