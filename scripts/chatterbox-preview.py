#!/usr/bin/env python3
"""Render a few Chatterbox Multilingual voice-clone previews."""

from __future__ import annotations

import argparse
import subprocess
from pathlib import Path

import torch
import torchaudio
from chatterbox.mtl_tts import ChatterboxMultilingualTTS


VARIANTS = (
    ("balanced", 0.50, 0.50),
    ("chronicler", 0.70, 0.30),
    ("restrained", 0.35, 0.40),
)
VARIANT_NAMES = tuple(variant[0] for variant in VARIANTS)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Clone a reference voice and render three Chatterbox previews."
    )
    parser.add_argument(
        "--reference",
        required=True,
        type=Path,
        action="append",
        help="Reference recording; repeat to compare several voices in one model load.",
    )
    text_source = parser.add_mutually_exclusive_group(required=True)
    text_source.add_argument("--text")
    text_source.add_argument(
        "--text-file",
        type=Path,
        help="Read UTF-8 text from a file; preferred for explicit stress marks.",
    )
    parser.add_argument("--language", default="ru")
    parser.add_argument(
        "--variant",
        action="append",
        choices=VARIANT_NAMES,
        help="Render only this delivery variant; repeat to select several.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("voicepacks/reborn-narrator/generated/chatterbox"),
    )
    return parser.parse_args()


def encode_ogg(source: Path, destination: Path) -> None:
    subprocess.run(
        [
            "ffmpeg",
            "-hide_banner",
            "-loglevel",
            "error",
            "-y",
            "-i",
            str(source),
            "-c:a",
            "libopus",
            "-b:a",
            "96k",
            str(destination),
        ],
        check=True,
    )


def main() -> int:
    args = parse_args()
    references = [reference.resolve() for reference in args.reference]
    output_dir = args.output_dir.resolve()
    text = (
        args.text_file.resolve().read_text(encoding="utf-8").strip()
        if args.text_file
        else args.text
    )

    missing = [reference for reference in references if not reference.is_file()]
    if missing:
        raise SystemExit(f"reference audio does not exist: {missing[0]}")
    if not torch.cuda.is_available():
        raise SystemExit("CUDA-enabled PyTorch is required for practical preview rendering")

    output_dir.mkdir(parents=True, exist_ok=True)
    device = torch.device("cuda")
    print(f"Loading Chatterbox Multilingual on {torch.cuda.get_device_name(0)}...")
    model = ChatterboxMultilingualTTS.from_pretrained(device=device)

    selected_variants = (
        [variant for variant in VARIANTS if variant[0] in args.variant]
        if args.variant
        else VARIANTS
    )
    compare_voices = len(references) > 1
    for reference in references:
        reference_output_dir = (
            output_dir / reference.stem if compare_voices else output_dir
        )
        reference_output_dir.mkdir(parents=True, exist_ok=True)
        print(f"Cloning reference: {reference}")
        for name, exaggeration, cfg_weight in selected_variants:
            print(
                f"Rendering {name}: exaggeration={exaggeration}, "
                f"cfg_weight={cfg_weight}..."
            )
            audio = model.generate(
                text,
                language_id=args.language,
                audio_prompt_path=str(reference),
                exaggeration=exaggeration,
                cfg_weight=cfg_weight,
            )
            wav_path = reference_output_dir / f"{name}.wav"
            ogg_path = reference_output_dir / f"{name}.ogg"
            torchaudio.save(str(wav_path), audio, model.sr)
            encode_ogg(wav_path, ogg_path)
            print(f"  {ogg_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
