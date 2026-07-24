#!/usr/bin/env python3
"""Generate and review reproducible music candidates for the Reborn theme."""

from __future__ import annotations

import argparse
import html
import json
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = REPO_ROOT / "art" / "reborn" / "source" / "music" / "soundtrack.json"
DEFAULT_OUTPUT = REPO_ROOT / "tmp" / "reborn-music"
DEFAULT_ACE_ROOT = Path.home() / ".codex" / "tools" / "ACE-Step-1.5"


def fail(message: str) -> "NoReturn":
    raise SystemExit(message)


def load_manifest(path: Path) -> dict[str, Any]:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema_version") != 1:
        fail(f"unsupported soundtrack manifest: {path}")
    return data


def run(command: list[str]) -> None:
    print("+", subprocess.list2cmdline(command), flush=True)
    subprocess.run(command, check=True)


def normalize_preview(source: Path, destination: Path) -> None:
    ffmpeg = shutil.which("ffmpeg")
    if not ffmpeg:
        fail("ffmpeg is required to create review previews")
    destination.parent.mkdir(parents=True, exist_ok=True)
    run(
        [
            ffmpeg,
            "-hide_banner",
            "-loglevel",
            "error",
            "-y",
            "-i",
            str(source),
            "-af",
            "loudnorm=I=-16:TP=-1.5:LRA=11",
            "-c:a",
            "libvorbis",
            "-q:a",
            "6",
            str(destination),
        ]
    )


def write_review(cue_id: str, cue: dict[str, Any], output_root: Path) -> Path:
    cue_dir = output_root / cue_id
    cards: list[str] = []
    for candidate in cue["seeds"]:
        candidate_id = candidate["id"]
        preview = cue_dir / f"{candidate_id}.ogg"
        if not preview.exists():
            continue
        concept = candidate.get("concept", candidate_id)
        prompt = candidate.get("prompt", cue["prompt"])
        cards.append(
            f"""
            <article>
              <h2>{html.escape(concept)}</h2>
              <p><code>{html.escape(candidate_id)}</code> · seed <code>{candidate["seed"]}</code></p>
              <audio controls preload="metadata" src="{html.escape(preview.name)}"></audio>
              <details><summary>Промпт варианта</summary><p>{html.escape(prompt)}</p></details>
            </article>
            """
        )

    review_path = cue_dir / "review.html"
    review_path.write_text(
        f"""<!doctype html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{html.escape(cue["title"])} — review</title>
<style>
  :root {{ color-scheme: dark; font-family: Georgia, serif; }}
  body {{ max-width: 980px; margin: 3rem auto; padding: 0 1.5rem; background: #090a0b; color: #eee4ca; }}
  h1 {{ font-size: 2.4rem; margin-bottom: .25rem; }}
  .meta {{ color: #b9a982; }}
  article {{ border: 1px solid #685a3a; background: #121313; padding: 1.3rem; margin: 1rem 0; }}
  article h2 {{ margin-top: 0; color: #d7bd79; }}
  audio {{ width: 100%; }}
  code {{ color: #d7bd79; }}
  details {{ margin-top: 2rem; color: #c9bea4; }}
</style>
</head>
<body>
  <h1>{html.escape(cue["title"])}</h1>
  <p class="meta">{cue["duration"]} сек · {cue["bpm"]} BPM · {html.escape(cue["keyscale"])} · {html.escape(cue["timesignature"])}</p>
  <p>Два камерных варианта с редкими нотами и длинными паузами. Пока выбираем направление и тембр; финальную форму и петлю уточним после прослушивания.</p>
  {''.join(cards)}
  <details><summary>Общее музыкальное задание</summary><p>{html.escape(cue["prompt"])}</p></details>
</body>
</html>
""",
        encoding="utf-8",
    )
    return review_path


def generate(
    cue_id: str,
    manifest_path: Path,
    output_root: Path,
    ace_root: Path,
    force: bool,
) -> None:
    manifest = load_manifest(manifest_path)
    cue = manifest["cues"].get(cue_id)
    if cue is None:
        fail(f"unknown cue: {cue_id}")
    if not (ace_root / "acestep").is_dir():
        fail(f"ACE-Step checkout not found: {ace_root}")

    sys.path.insert(0, str(ace_root))
    from acestep.handler import AceStepHandler
    from acestep.inference import GenerationConfig, GenerationParams, generate_music

    generator = manifest["generator"]
    cue_dir = output_root / cue_id
    raw_dir = cue_dir / "lossless"
    raw_dir.mkdir(parents=True, exist_ok=True)

    print(f"Loading {generator['model']} from {ace_root}", flush=True)
    started = time.monotonic()
    handler = AceStepHandler()
    status, success = handler.initialize_service(
        project_root=str(ace_root),
        config_path=generator["model"],
        device="cuda",
        offload_to_cpu=False,
    )
    if not success:
        fail(f"ACE-Step initialization failed: {status}")
    print(f"Model ready in {time.monotonic() - started:.1f}s: {status}", flush=True)

    for candidate in cue["seeds"]:
        candidate_id = candidate["id"]
        lossless = raw_dir / f"{candidate_id}.flac"
        preview = cue_dir / f"{candidate_id}.ogg"
        if lossless.exists() and preview.exists() and not force:
            print(f"Keeping existing {candidate_id}", flush=True)
            continue

        params = GenerationParams(
            task_type="text2music",
            thinking=False,
            use_cot_caption=False,
            use_cot_language=False,
            use_cot_metas=False,
            caption=candidate.get("prompt", cue["prompt"]),
            lyrics="[Instrumental]",
            instrumental=True,
            vocal_language="unknown",
            bpm=cue["bpm"],
            keyscale=cue["keyscale"],
            timesignature=cue["timesignature"],
            duration=cue["duration"],
            inference_steps=generator["inference_steps"],
            guidance_scale=generator.get("guidance_scale", 1.0),
            shift=generator["shift"],
            seed=candidate["seed"],
        )
        config = GenerationConfig(
            batch_size=1,
            use_random_seed=False,
            seeds=[candidate["seed"]],
            audio_format="flac",
        )

        candidate_start = time.monotonic()
        result = generate_music(
            handler,
            None,
            params=params,
            config=config,
            save_dir=str(raw_dir),
        )
        if not result.success or not result.audios:
            fail(f"{candidate_id} generation failed: {result.status_message}")
        generated_path = Path(result.audios[0]["path"])
        if not generated_path.exists():
            fail(f"{candidate_id} returned no saved audio file")
        if lossless.exists():
            lossless.unlink()
        generated_path.replace(lossless)
        normalize_preview(lossless, preview)
        print(
            f"Generated {candidate_id} in {time.monotonic() - candidate_start:.1f}s",
            flush=True,
        )

    review_path = write_review(cue_id, cue, output_root)
    print(f"Review: {review_path}", flush=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=("generate", "review"))
    parser.add_argument("cue")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--ace-root", type=Path, default=DEFAULT_ACE_ROOT)
    parser.add_argument("--force", action="store_true")
    args = parser.parse_args()

    manifest = load_manifest(args.manifest)
    cue = manifest["cues"].get(args.cue)
    if cue is None:
        fail(f"unknown cue: {args.cue}")
    if args.command == "review":
        print(write_review(args.cue, cue, args.output))
        return
    generate(args.cue, args.manifest, args.output, args.ace_root, args.force)


if __name__ == "__main__":
    main()
