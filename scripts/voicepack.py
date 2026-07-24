#!/usr/bin/env python3
"""Generate and validate replaceable voice packs for Four Winds Reborn."""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import tempfile
from dataclasses import dataclass
from pathlib import Path
from typing import Any


SCENE_RE = re.compile(r"^###\s+(\d{2})\s+—")
LANGUAGE_NAMES = {"ru": "Russian", "en": "English"}


@dataclass(frozen=True)
class Line:
    scene: int
    language: str
    text: str
    duration_ms: int
    id: str = ""
    speaker: str = ""
    max_duration_ms: int = 0
    synthesis_text: str = ""
    trim_after_pause_ms: int = 0


def fail(message: str) -> "NoReturn":
    raise SystemExit(message)


def run(command: list[str]) -> None:
    printable = subprocess.list2cmdline(command)
    print(f"+ {printable}")
    subprocess.run(command, check=True)


def tool(name: str) -> str:
    found = shutil.which(name)
    if not found:
        fail(f"required executable is not available: {name}")
    return found


def load_manifest(path: Path) -> tuple[dict[str, Any], Path]:
    path = path.resolve()
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema_version") != 1:
        fail(f"unsupported voice pack schema in {path}")
    return data, path.parent


def resolve(base: Path, value: str) -> Path:
    return (base / value).resolve()


def parse_intro_markdown(path: Path) -> dict[int, dict[str, str]]:
    scenes: dict[int, dict[str, str]] = {}
    current_scene: int | None = None
    current_language: str | None = None

    for raw_line in path.read_text(encoding="utf-8").splitlines():
        match = SCENE_RE.match(raw_line)
        if match:
            current_scene = int(match.group(1))
            scenes[current_scene] = {}
            current_language = None
            continue

        language = None
        if raw_line.startswith("**RU:**"):
            language = "ru"
        elif raw_line.startswith("**EN:**"):
            language = "en"

        if language:
            if current_scene is None:
                fail(f"localized text appears before a scene heading in {path}")
            current_language = language
            scenes[current_scene][language] = raw_line.split("**", 2)[-1].strip()
            continue

        if not raw_line.strip():
            current_language = None
        elif current_scene is not None and current_language is not None:
            previous = scenes[current_scene][current_language]
            scenes[current_scene][current_language] = f"{previous} {raw_line.strip()}"

    return scenes


def load_lines(manifest: dict[str, Any], base: Path) -> list[Line]:
    source = manifest["source"]
    if source["type"] == "hero_calls_json":
        source_path = resolve(base, source["path"])
        data = json.loads(source_path.read_text(encoding="utf-8"))
        speakers = data.get("speakers", [])
        calls = data.get("calls", [])
        expected_count = int(source["line_count"])
        expected_scene_count = int(source.get("scene_count", expected_count))
        lines: list[Line] = []
        scene = 0
        for speaker in speakers:
            speaker_id = str(speaker["id"]).strip()
            if not speaker_id:
                fail(f"hero call speaker has no id in {source_path}")
            for call in calls:
                call_id = str(call["id"]).strip()
                if not call_id:
                    fail(f"hero call has no id in {source_path}")
                scene += 1
                for language in ("ru", "en"):
                    localized = call.get(language)
                    if isinstance(localized, str):
                        localized = {"text": localized}
                    if not isinstance(localized, dict):
                        fail(
                            f"hero call {call_id} has no {language} text "
                            f"in {source_path}"
                        )
                    text = str(localized.get("text", "")).strip()
                    if not text:
                        fail(
                            f"hero call {call_id} has empty {language} text "
                            f"in {source_path}"
                        )
                    lines.append(
                        Line(
                            scene=scene,
                            language=language,
                            text=text,
                            duration_ms=0,
                            id=f"{speaker_id}_{call_id}",
                            speaker=speaker_id,
                            max_duration_ms=max(
                                0, int(localized.get("max_duration_ms", 0))
                            ),
                            synthesis_text=str(
                                localized.get("synthesis_text", "")
                            ).strip(),
                            trim_after_pause_ms=max(
                                0,
                                int(localized.get("trim_after_pause_ms", 0)),
                            ),
                        )
                    )
        if len(lines) != expected_count:
            fail(
                f"expected {expected_count} hero call lines in {source_path}, "
                f"got {len(lines)}"
            )
        if scene != expected_scene_count:
            fail(
                f"expected {expected_scene_count} hero call scenes in "
                f"{source_path}, got {scene}"
            )
        return sorted(lines, key=lambda line: (line.scene, line.language))

    if source["type"] == "voice_lines_json":
        source_path = resolve(base, source["path"])
        data = json.loads(source_path.read_text(encoding="utf-8"))
        raw_lines = data.get("lines", [])
        expected_count = int(source["line_count"])
        expected_scene_count = int(source.get("scene_count", expected_count))
        if len(raw_lines) != expected_count:
            fail(
                f"expected {expected_count} voice lines in {source_path}, "
                f"got {len(raw_lines)}"
            )

        lines: list[Line] = []
        expected_scenes = set(range(1, expected_scene_count + 1))
        actual_scenes: set[int] = set()
        actual_keys: set[tuple[int, str]] = set()
        for raw_line in raw_lines:
            scene = int(raw_line["scene"])
            language = str(raw_line["language"])
            text = str(raw_line["text"]).strip()
            if language not in LANGUAGE_NAMES:
                fail(f"unsupported language code in {source_path}: {language}")
            if not text:
                fail(f"voice line {scene:02d} has no text in {source_path}")
            key = (scene, language)
            if key in actual_keys:
                fail(
                    f"duplicate voice line {scene:02d}/{language} in {source_path}"
                )
            actual_keys.add(key)
            actual_scenes.add(scene)
            lines.append(
                Line(
                    scene=scene,
                    language=language,
                    text=text,
                    duration_ms=0,
                    id=str(raw_line.get("id", "")).strip(),
                    speaker=str(raw_line.get("speaker", "")).strip(),
                    max_duration_ms=max(0, int(raw_line.get("max_duration_ms", 0))),
                    synthesis_text=str(raw_line.get("synthesis_text", "")).strip(),
                    trim_after_pause_ms=max(
                        0, int(raw_line.get("trim_after_pause_ms", 0))
                    ),
                )
            )

        if actual_scenes != expected_scenes:
            fail(
                f"line set differs: expected {sorted(expected_scenes)}, "
                f"got {sorted(actual_scenes)}"
            )
        return sorted(lines, key=lambda line: (line.scene, line.language))

    if source["type"] != "reborn_intro_markdown":
        fail(f"unsupported source type: {source['type']}")

    scenes = parse_intro_markdown(resolve(base, source["path"]))
    expected_count = int(source["scene_count"])
    expected_scenes = set(range(1, expected_count + 1))
    if set(scenes) != expected_scenes:
        fail(f"scene set differs: expected {sorted(expected_scenes)}, got {sorted(scenes)}")

    timing_path = resolve(base, manifest["target"]["timing_file"])
    timing = json.loads(timing_path.read_text(encoding="utf-8"))
    frames = timing.get("frames", [])
    if len(frames) != expected_count:
        fail(f"expected {expected_count} timing frames in {timing_path}, got {len(frames)}")

    lines: list[Line] = []
    for scene in range(1, expected_count + 1):
        for language in ("ru", "en"):
            text = scenes[scene].get(language, "").strip()
            if not text:
                fail(f"scene {scene:02d} has no {language} text")
            lines.append(
                Line(
                    scene=scene,
                    language=language,
                    text=text,
                    duration_ms=int(
                        frames[scene - 1].get(
                            f"duration_ms:{language}",
                            frames[scene - 1]["duration_ms"],
                        )
                    ),
                    max_duration_ms=0,
                    synthesis_text="",
                    trim_after_pause_ms=0,
                )
            )
    return lines


def output_path(manifest: dict[str, Any], base: Path, line: Line) -> Path:
    target = manifest["target"]
    filename = target["filename"].format(
        language=line.language,
        scene=line.scene,
        id=line.id,
        speaker=line.speaker,
    )
    return resolve(base, target["directory"]) / filename


def generated_dir(base: Path) -> Path:
    return base / "generated"


def spoken_text(manifest: dict[str, Any], language: str, text: str) -> str:
    replacements = manifest.get("pronunciation", {}).get(language, {})
    for source, replacement in replacements.items():
        text = text.replace(source, replacement)
    return text


def language_code(language_name: str) -> str:
    for code, known_name in LANGUAGE_NAMES.items():
        if known_name == language_name:
            return code
    fail(f"unsupported language name: {language_name}")


def reference_config(
    manifest: dict[str, Any],
    language: str,
    speaker: str = "",
) -> dict[str, Any]:
    if speaker:
        speaker_config = manifest.get("speakers", {}).get(speaker)
        if speaker_config is None:
            fail(f"voice pack has no reference profile for speaker: {speaker}")
        return speaker_config.get("references", {}).get(
            language,
            speaker_config.get("reference", manifest["reference"]),
        )
    return manifest.get("references", {}).get(language, manifest["reference"])


def reference_path(
    manifest: dict[str, Any],
    base: Path,
    language: str,
    speaker: str = "",
    explicit_reference: str | None = None,
) -> Path:
    if explicit_reference:
        return Path(explicit_reference).resolve()
    config = reference_config(manifest, language, speaker)
    return (generated_dir(base) / config.get("audio", "reference.wav")).resolve()


def ffprobe_duration(path: Path) -> float:
    result = subprocess.run(
        [
            tool("ffprobe"),
            "-v",
            "error",
            "-show_entries",
            "format=duration",
            "-of",
            "default=noprint_wrappers=1:nokey=1",
            str(path),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    return float(result.stdout.strip())


def pause_cut_ms(
    source: Path,
    search_after_ms: int,
    pause_duration_ms: int = 140,
) -> int:
    """Return the start of the first real pause after the requested timestamp."""
    result = subprocess.run(
        [
            tool("ffmpeg"),
            "-hide_banner",
            "-i",
            str(source),
            "-af",
            (
                "silencedetect="
                f"noise=-42dB:d={pause_duration_ms / 1000.0:.3f}"
            ),
            "-f",
            "null",
            "-",
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    for match in re.finditer(r"silence_start:\s*([0-9.]+)", result.stderr):
        candidate_ms = round(float(match.group(1)) * 1000)
        if candidate_ms >= search_after_ms:
            return candidate_ms
    return 0


def prepare_audio(
    source: Path,
    destination: Path,
    audio: dict[str, Any],
    max_duration_ms: int = 0,
    trim_after_pause_ms: int = 0,
) -> float:
    destination.parent.mkdir(parents=True, exist_ok=True)
    ffmpeg = tool("ffmpeg")

    with tempfile.TemporaryDirectory(prefix="four-winds-voice-") as temp_dir:
        normalized = Path(temp_dir) / "normalized.wav"
        detected_cut_ms = (
            pause_cut_ms(source, trim_after_pause_ms)
            if trim_after_pause_ms > 0
            else 0
        )
        pause_postroll_ms = max(
            0, int(audio.get("pause_postroll_ms", 0))
        )
        detected_or_limit_ms = (
            detected_cut_ms + pause_postroll_ms
            if detected_cut_ms
            else max_duration_ms
        )
        cut_ms = (
            min(detected_or_limit_ms, max_duration_ms)
            if detected_or_limit_ms > 0 and max_duration_ms > 0
            else detected_or_limit_ms
        )
        trim_filter = (
            f"atrim=end={cut_ms / 1000.0:.3f},"
            if cut_ms > 0
            else ""
        )
        if detected_cut_ms:
            print(
                f"    pause trim: {detected_cut_ms / 1000.0:.3f} s "
                f"+ {pause_postroll_ms} ms post-roll "
                f"(searched after {trim_after_pause_ms / 1000.0:.3f} s)"
            )
        start_trim = (
            ""
            if detected_cut_ms
            else (
                "silenceremove="
                "start_periods=1:start_duration=0.04:start_threshold=-45dB,"
            )
        )
        end_trim = (
            ""
            if detected_cut_ms or not bool(audio.get("trim_end", True))
            else (
                "areverse,"
                "silenceremove="
                "start_periods=1:"
                f"start_duration={max(0.01, float(audio.get('end_trim_duration_ms', 80)) / 1000.0):.3f}:"
                f"start_threshold={float(audio.get('end_trim_threshold_db', -55)):.1f}dB:"
                f"start_silence={max(0.0, float(audio.get('end_trim_keep_ms', 120)) / 1000.0):.3f},"
                "areverse,"
            )
        )
        filter_chain = (
            trim_filter
            + start_trim
            + end_trim
            + f"loudnorm=I={audio['loudness_lufs']}:"
            f"TP={audio['true_peak_db']}:LRA=11"
        )
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
                filter_chain,
                "-ac",
                str(audio["channels"]),
                "-ar",
                str(audio["sample_rate"]),
                str(normalized),
            ]
        )

        tail_silence = max(0.0, float(audio.get("tail_silence_ms", 0)) / 1000.0)
        final_filter = f"apad=pad_dur={tail_silence:.3f}" if tail_silence else "anull"
        run(
            [
                ffmpeg,
                "-hide_banner",
                "-loglevel",
                "error",
                "-y",
                "-i",
                str(normalized),
                "-af",
                final_filter,
                "-ac",
                str(audio["channels"]),
                "-ar",
                str(audio["sample_rate"]),
                "-c:a",
                "libvorbis",
                "-q:a",
                str(audio["vorbis_quality"]),
                str(destination),
            ]
        )
    return ffprobe_duration(destination)


def update_timing_file(
    manifest: dict[str, Any],
    base: Path,
    durations: dict[tuple[str, int], float],
) -> None:
    if not durations:
        return
    if "timing_file" not in manifest["target"]:
        return

    timing_path = resolve(base, manifest["target"]["timing_file"])
    timing = json.loads(timing_path.read_text(encoding="utf-8"))
    frames = timing.get("frames", [])
    expected_count = int(manifest["source"]["scene_count"])
    if len(frames) != expected_count:
        fail(f"expected {expected_count} timing frames in {timing_path}, got {len(frames)}")

    for (language, scene), duration in durations.items():
        frames[scene - 1][f"duration_ms:{language}"] = max(1, round(duration * 1000.0))

    timing_path.write_text(
        json.dumps(timing, ensure_ascii=False, indent=4) + "\n",
        encoding="utf-8",
    )
    print(f"Updated localized intro timings: {timing_path}")


def selected_lines(
    manifest: dict[str, Any],
    base: Path,
    args: argparse.Namespace,
) -> list[Line]:
    lines = load_lines(manifest, base)
    if args.language != "all":
        lines = [line for line in lines if line.language == args.language]
    selected_speakers = set(getattr(args, "speaker", []) or [])
    if selected_speakers:
        lines = [line for line in lines if line.speaker in selected_speakers]
    if args.scene:
        selected_scenes = set(args.scene)
        lines = [line for line in lines if line.scene in selected_scenes]
    if not lines:
        fail("no lines selected")
    return lines


def candidate(manifest: dict[str, Any], candidate_id: str) -> dict[str, str]:
    for item in manifest["candidates"]:
        if item["id"] == candidate_id:
            return item
    available = ", ".join(item["id"] for item in manifest["candidates"])
    fail(f"unknown candidate '{candidate_id}'; choose one of: {available}")


def load_qwen_model(model_name: str, attention: str):
    try:
        import torch
        from qwen_tts import Qwen3TTSModel
    except ImportError as error:
        fail(
            "Qwen3-TTS is not installed in this Python environment. "
            "Run scripts/setup-voice-tools.ps1 first.\n"
            f"Import error: {error}"
        )

    if not torch.cuda.is_available():
        fail("Qwen3-TTS preview/render requires a CUDA GPU")

    dtype = torch.bfloat16 if torch.cuda.is_bf16_supported() else torch.float16
    print(f"Loading {model_name} on CUDA ({dtype}, attention={attention})")
    return Qwen3TTSModel.from_pretrained(
        model_name,
        device_map="cuda:0",
        dtype=dtype,
        attn_implementation=attention,
    )


def load_chatterbox_model():
    try:
        import torch
        from chatterbox.mtl_tts import ChatterboxMultilingualTTS
    except ImportError as error:
        fail(
            "Chatterbox Multilingual is not installed. "
            "Run this command with .venv-chatterbox.\n"
            f"Import error: {error}"
        )

    if not torch.cuda.is_available():
        fail("CUDA-enabled PyTorch is required for practical Chatterbox rendering")
    print(f"Loading Chatterbox Multilingual on {torch.cuda.get_device_name(0)}...")
    return ChatterboxMultilingualTTS.from_pretrained(device=torch.device("cuda"))


def command_list(manifest: dict[str, Any], base: Path, _: argparse.Namespace) -> None:
    lines = load_lines(manifest, base)
    for line in lines:
        destination = output_path(manifest, base, line)
        identity = f" [{line.speaker}]" if line.speaker else ""
        print(
            f"{line.language} {line.scene:02d}{identity}  "
            f"{line.duration_ms:5d} ms  {destination.name}\n"
            f"    {line.text}"
        )
    print(f"\n{len(lines)} localized lines")


def command_preview(manifest: dict[str, Any], base: Path, args: argparse.Namespace) -> None:
    selected_candidates = (
        manifest["candidates"]
        if args.all
        else [candidate(manifest, candidate_id) for candidate_id in args.candidate]
    )
    preview_dir = generated_dir(base) / "previews"
    preview_dir.mkdir(parents=True, exist_ok=True)
    existing = [
        preview_dir / f"{selected['id']}.ogg"
        for selected in selected_candidates
        if (preview_dir / f"{selected['id']}.wav").exists()
        or (preview_dir / f"{selected['id']}.ogg").exists()
    ]
    if existing and not args.force:
        fail(f"preview already exists: {existing[0]}; pass --force to replace it")

    backend = manifest["backend"]
    model = load_qwen_model(backend["design_model"], backend["attention"])
    reference = manifest["reference"]

    import soundfile as sf

    for index, selected in enumerate(selected_candidates, 1):
        preview_language = selected.get("language", reference["language"])
        preview_code = language_code(preview_language)
        preview_text = spoken_text(
            manifest,
            preview_code,
            selected.get("preview_text", reference["text"]),
        )
        print(
            f"[{index}/{len(selected_candidates)}] Designing "
            f"{selected['name']} ({selected['id']})"
        )
        wavs, sample_rate = model.generate_voice_design(
            text=preview_text,
            language=preview_language,
            instruct=selected["instruct"],
        )
        wav_path = preview_dir / f"{selected['id']}.wav"
        ogg_path = preview_dir / f"{selected['id']}.ogg"
        sf.write(wav_path, wavs[0], sample_rate)
        duration = prepare_audio(wav_path, ogg_path, manifest["audio"])
        print(f"    Preview ready ({duration:.2f} s): {ogg_path}")


def command_select(manifest: dict[str, Any], base: Path, args: argparse.Namespace) -> None:
    selected = candidate(manifest, args.candidate)
    source = generated_dir(base) / "previews" / f"{selected['id']}.wav"
    selected_language_name = selected.get(
        "language",
        manifest["reference"]["language"],
    )
    selected_language = language_code(selected_language_name)
    selected_speaker = selected.get("speaker", "")
    selected_reference = reference_config(
        manifest,
        selected_language,
        selected_speaker,
    )
    destination = (
        generated_dir(base)
        / selected_reference.get("audio", "reference.wav")
    ).resolve()
    if not source.is_file():
        fail(f"generate the preview first: {source}")
    if destination.exists() and not args.force:
        fail(f"selected reference already exists: {destination}; pass --force to replace it")
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    metadata = destination.with_suffix(".json")
    selected_text = selected.get(
        "preview_text",
        selected_reference["text"],
    )
    metadata.write_text(
        json.dumps(
            {
                "candidate": selected["id"],
                "name": selected["name"],
                "reference_text": selected_text,
                "spoken_reference_text": spoken_text(
                    manifest,
                    selected_language,
                    selected_text,
                ),
                "reference_language": selected_language_name,
                "speaker": selected_speaker,
            },
            ensure_ascii=False,
            indent=2,
        )
        + "\n",
        encoding="utf-8",
    )
    print(f"Selected voice reference: {destination}")


def command_pronunciation_preview(
    manifest: dict[str, Any],
    base: Path,
    args: argparse.Namespace,
) -> None:
    trials = manifest.get("pronunciation_trials", [])
    if not trials:
        fail("manifest has no pronunciation_trials")
    if args.trial:
        selected_ids = set(args.trial)
        known_ids = {trial["id"] for trial in trials}
        unknown_ids = selected_ids - known_ids
        if unknown_ids:
            fail(
                "unknown pronunciation trial(s): "
                + ", ".join(sorted(unknown_ids))
            )
        trials = [trial for trial in trials if trial["id"] in selected_ids]

    trial_language = language_code(trials[0]["language"])
    reference = reference_path(manifest, base, trial_language)
    if not reference.is_file():
        fail(
            f"selected reference is missing: {reference}\n"
            "Generate previews and run the select command first."
        )

    preview_dir = generated_dir(base) / "pronunciation"
    preview_dir.mkdir(parents=True, exist_ok=True)
    existing = [
        preview_dir / f"{trial['id']}.ogg"
        for trial in trials
        if (preview_dir / f"{trial['id']}.wav").exists()
        or (preview_dir / f"{trial['id']}.ogg").exists()
    ]
    if existing and not args.force:
        fail(
            f"pronunciation preview already exists: {existing[0]}; "
            "pass --force to replace it"
        )

    backend = manifest["backend"]
    model = load_qwen_model(backend["clone_model"], backend["attention"])
    prompt = model.create_voice_clone_prompt(
        ref_audio=str(reference),
        x_vector_only_mode=True,
    )

    import soundfile as sf

    for index, trial in enumerate(trials, 1):
        print(
            f"[{index}/{len(trials)}] Pronunciation trial "
            f"{trial['name']} ({trial['id']})"
        )
        wavs, sample_rate = model.generate_voice_clone(
            text=trial["text"],
            language=trial["language"],
            voice_clone_prompt=prompt,
        )
        wav_path = preview_dir / f"{trial['id']}.wav"
        ogg_path = preview_dir / f"{trial['id']}.ogg"
        sf.write(wav_path, wavs[0], sample_rate)
        duration = prepare_audio(wav_path, ogg_path, manifest["audio"])
        print(f"    Preview ready ({duration:.2f} s): {ogg_path}")


def command_render(manifest: dict[str, Any], base: Path, args: argparse.Namespace) -> None:
    lines = selected_lines(manifest, base, args)

    references = {
        (line.speaker, line.language): reference_path(
            manifest,
            base,
            line.language,
            line.speaker,
            args.reference,
        )
        for line in lines
    }
    for reference in references.values():
        if not reference.is_file():
            fail(
                f"selected reference is missing: {reference}\n"
                "Generate previews and run the select command first."
            )

    existing = [output_path(manifest, base, line) for line in lines if output_path(manifest, base, line).exists()]
    if existing and not args.force:
        fail(
            f"{len(existing)} target files already exist (for example {existing[0]}). "
            "Pass --force only after choosing the final voice."
        )

    raw_dir = generated_dir(base) / "raw"
    raw_dir.mkdir(parents=True, exist_ok=True)
    render_backend = manifest.get("render_backend", manifest["backend"])
    render_type = render_backend.get("type", "qwen3_tts")

    if render_type == "chatterbox_multilingual":
        model = load_chatterbox_model()
        import torchaudio

        durations: dict[tuple[str, int], float] = {}
        for index, line in enumerate(lines, 1):
            print(
                f"[{index}/{len(lines)}] Rendering "
                f"{line.language} scene {line.scene:02d}",
                flush=True,
            )
            audio = model.generate(
                spoken_text(
                    manifest,
                    line.language,
                    line.synthesis_text or line.text,
                ),
                language_id=line.language,
                audio_prompt_path=str(references[(line.speaker, line.language)]),
                exaggeration=float(render_backend.get("exaggeration", 0.5)),
                cfg_weight=float(render_backend.get("cfg_weight", 0.5)),
            )
            raw_path = raw_dir / f"{line.language}_{line.scene:02d}.wav"
            torchaudio.save(str(raw_path), audio, model.sr)
            destination = output_path(manifest, base, line)
            duration = prepare_audio(
                raw_path,
                destination,
                manifest["audio"],
                line.max_duration_ms,
                line.trim_after_pause_ms,
            )
            durations[(line.language, line.scene)] = duration
            print(
                f"    {destination.name}: {duration:.2f} s natural duration",
                flush=True,
            )
        update_timing_file(manifest, base, durations)
        return

    if render_type != "qwen3_tts":
        fail(f"unsupported render backend: {render_type}")

    model = load_qwen_model(render_backend["clone_model"], render_backend["attention"])
    prompts = {
        identity: model.create_voice_clone_prompt(
            ref_audio=str(reference),
            # The ICL transcript must match the promoted reference recording
            # exactly. Pronunciation rewrites apply to generated lines only.
            ref_text=reference_config(manifest, identity[1], identity[0])["text"],
            x_vector_only_mode=False,
        )
        for identity, reference in references.items()
    }
    import soundfile as sf

    durations: dict[tuple[str, int], float] = {}
    for index, line in enumerate(lines, 1):
        print(f"[{index}/{len(lines)}] Rendering {line.language} scene {line.scene:02d}")
        wavs, sample_rate = model.generate_voice_clone(
            text=spoken_text(
                manifest,
                line.language,
                line.synthesis_text or line.text,
            ),
            language=LANGUAGE_NAMES[line.language],
            voice_clone_prompt=prompts[(line.speaker, line.language)],
        )
        raw_path = raw_dir / f"{line.language}_{line.scene:02d}.wav"
        sf.write(raw_path, wavs[0], sample_rate)
        destination = output_path(manifest, base, line)
        duration = prepare_audio(
            raw_path,
            destination,
            manifest["audio"],
            line.max_duration_ms,
            line.trim_after_pause_ms,
        )
        durations[(line.language, line.scene)] = duration
        print(
            f"    {destination.name}: {duration:.2f} s natural duration"
        )
    update_timing_file(manifest, base, durations)


def command_finalize(manifest: dict[str, Any], base: Path, args: argparse.Namespace) -> None:
    lines = selected_lines(manifest, base, args)
    raw_dir = generated_dir(base) / "raw"
    durations: dict[tuple[str, int], float] = {}

    for index, line in enumerate(lines, 1):
        raw_path = raw_dir / f"{line.language}_{line.scene:02d}.wav"
        if not raw_path.is_file():
            fail(
                f"raw synthesis is missing: {raw_path}\n"
                "Use render only for the missing line; existing raw lines do not need synthesis."
            )
        destination = output_path(manifest, base, line)
        print(
            f"[{index}/{len(lines)}] Finalizing existing "
            f"{line.language} scene {line.scene:02d}"
        )
        duration = prepare_audio(
            raw_path,
            destination,
            manifest["audio"],
            line.max_duration_ms,
            line.trim_after_pause_ms,
        )
        durations[(line.language, line.scene)] = duration
        print(f"    {destination.name}: {duration:.2f} s natural duration")

    update_timing_file(manifest, base, durations)


def command_sync_timings(manifest: dict[str, Any], base: Path, args: argparse.Namespace) -> None:
    lines = selected_lines(manifest, base, args)
    durations: dict[tuple[str, int], float] = {}

    for line in lines:
        path = output_path(manifest, base, line)
        if not path.is_file():
            fail(f"cannot measure missing final audio: {path}")
        durations[(line.language, line.scene)] = ffprobe_duration(path)

    update_timing_file(manifest, base, durations)


def command_verify(manifest: dict[str, Any], base: Path, _: argparse.Namespace) -> None:
    lines = load_lines(manifest, base)
    failures = 0
    tail_silence = max(
        0.0, float(manifest["audio"].get("tail_silence_ms", 0)) / 1000.0
    )
    for line in lines:
        path = output_path(manifest, base, line)
        if not path.is_file():
            print(f"MISSING  {path}")
            failures += 1
            continue
        duration = ffprobe_duration(path)
        if line.max_duration_ms > 0:
            maximum = line.max_duration_ms / 1000.0 + tail_silence + 0.08
            if duration > maximum:
                print(
                    f"OVERLONG {path.name}: audio {duration:.3f} / "
                    f"maximum {maximum:.3f} s"
                )
                failures += 1
                continue
        if line.duration_ms <= 0:
            print(f"OK       {path.name}: audio {duration:.3f} s")
            continue
        declared = line.duration_ms / 1000.0
        status = "OK" if abs(duration - declared) <= 0.05 else "MISMATCH"
        print(f"{status:8s} {path.name}: audio {duration:.3f} / theme {declared:.3f} s")
        failures += status != "OK"
    if failures:
        fail(f"voice pack verification failed: {failures} problem(s)")
    print(f"Voice pack verified: {len(lines)} files")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("manifest", type=Path)
    subparsers = parser.add_subparsers(dest="command", required=True)

    subparsers.add_parser("list", help="show the localized line contract")

    preview = subparsers.add_parser("preview", help="design one or more voice previews")
    preview_selection = preview.add_mutually_exclusive_group(required=True)
    preview_selection.add_argument("--candidate", action="append")
    preview_selection.add_argument("--all", action="store_true")
    preview.add_argument("--force", action="store_true")

    select = subparsers.add_parser("select", help="select a generated preview as the voice identity")
    select.add_argument("--candidate", required=True)
    select.add_argument("--force", action="store_true")

    pronunciation = subparsers.add_parser(
        "pronunciation-preview",
        help="audition pronunciation spellings in the selected voice",
    )
    pronunciation.add_argument(
        "--trial",
        action="append",
        help="render only this pronunciation trial id (repeatable)",
    )
    pronunciation.add_argument("--force", action="store_true")

    render = subparsers.add_parser("render", help="render final localized OGG files")
    render.add_argument("--language", choices=("ru", "en", "all"), default="all")
    render.add_argument(
        "--speaker",
        action="append",
        help="render only this speaker id (repeatable)",
    )
    render.add_argument("--scene", type=int, action="append")
    render.add_argument("--reference")
    render.add_argument("--force", action="store_true")

    finalize = subparsers.add_parser(
        "finalize",
        help="rebuild final OGG files from existing raw synthesis without running TTS",
    )
    finalize.add_argument("--language", choices=("ru", "en", "all"), default="all")
    finalize.add_argument(
        "--speaker",
        action="append",
        help="finalize only this speaker id (repeatable)",
    )
    finalize.add_argument("--scene", type=int, action="append")

    sync_timings = subparsers.add_parser(
        "sync-timings",
        help="measure existing final OGG files and write localized frame durations",
    )
    sync_timings.add_argument("--language", choices=("ru", "en", "all"), default="all")
    sync_timings.add_argument(
        "--speaker",
        action="append",
        help="sync only this speaker id (repeatable)",
    )
    sync_timings.add_argument("--scene", type=int, action="append")

    subparsers.add_parser("verify", help="check filenames and frame durations")
    return parser


def main() -> None:
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
        sys.stderr.reconfigure(encoding="utf-8")
    args = build_parser().parse_args()
    manifest, base = load_manifest(args.manifest)
    commands = {
        "list": command_list,
        "preview": command_preview,
        "select": command_select,
        "pronunciation-preview": command_pronunciation_preview,
        "render": command_render,
        "finalize": command_finalize,
        "sync-timings": command_sync_timings,
        "verify": command_verify,
    }
    commands[args.command](manifest, base, args)


if __name__ == "__main__":
    main()
