#!/usr/bin/env python3

from __future__ import annotations

import json
import sys
from collections import Counter, defaultdict
from pathlib import Path


INDEX_TABLES = {
    "index:winds": "winds",
    "index:clans": "clans",
    "index:abilities": "abilities",
    "index:specials": "specials",
    "index:avatars": "avatars",
    "index:spells": "spells",
    "index:creatures": "creatures",
    "index:lands": "lands",
}

SPELL_TARGETS = {
    "friendly",
    "enemy",
    "any",
    "enemy_party",
    "land",
    "my_player",
    "other_player",
    "all_players",
}

AI_PROFILES = {"balanced", "aggressive", "economic", "control"}

CAST_SPELLS = {
    "cast_hellblast": "hell_blast",
    "cast_draw_number": "draw_number",
    "cast_draw_sword": "draw_sword",
    "cast_draw_skull": "draw_skull",
    "cast_random_discard": "random_discard",
    "cast_silence": "silence",
    "cast_scry_runes": "scry_runes",
    "cast_mana_fog": "mana_fog",
}

# These values are deliberate Reborn contracts. Manual differences and their
# rationale live in docs/RulesDecisions.md.
CANONICAL_CREATURE_PRICES = {
    "tornado": 240,
    "griffon": 170,
    "great_carol": 180,
}

CANONICAL_CLAN_NAMES = {
    "red": "Red",
    "yellow": "Yellow",
    "aqua": "Aqua",
    "purple": "Purple",
}

CANONICAL_SPELL_PRICES = {
    "demonic_compulsion": 30,
    "dispel_magic": 120,
    "heroism": 30,
}

CANONICAL_SPELL_OWNERS = {
    "mass_dispel": {"ziag", "javed"},
    "hell_blast": set(),
    "draw_number": set(),
    "draw_skull": set(),
    "draw_sword": set(),
    "random_discard": set(),
    "scry_runes": set(),
    "silence": set(),
    "mana_fog": set(),
}

CANONICAL_CREATURE_OWNERS = {
    "chameleon": {"ziag"},
    "griffon": {"javed"},
    "juggernaut": {"logun"},
    "shanahan": {"kierac"},
    "tornado": {"dayla", "nucrus"},
}

RUSSIAN_SOUND_RESOURCE_IDS = {
    "orachi_game": 1053,
    "lakkho_game": 1063,
    "dayla_game": 1073,
    "ziag_game": 1083,
    "niana_game": 1093,
    "kierac_game": 1103,
    "logun_kong": 1112,
    "logun_game": 1113,
    "nucrus_game": 1123,
    "javed_game": 1133,
    **{name: 1140 + offset for offset, name in enumerate((
        "orachi_name", "lakkho_name", "dayla_name", "ziag_name", "niana_name",
        "kierac_name", "logun_name", "nucrus_name", "javed_name",
    ))},
    **{f"creature{offset:02d}": 1149 + offset for offset in range(1, 30)},
    "red_name": 1190,
    "yellow_name": 1191,
    "aqua_name": 1192,
    "purple_name": 1193,
    "begin": 1200,
    "round_east": 1201,
    "round_south": 1202,
    "round_west": 1203,
    "round_north": 1204,
}


class ThemeValidator:
    def __init__(self, repository: Path, documents: dict[Path, object]):
        self.repository = repository
        self.documents = documents
        self.errors: list[str] = []

    def relative(self, path: Path) -> str:
        return path.relative_to(self.repository).as_posix()

    def error(self, path: Path, message: str) -> None:
        self.errors.append(f"{self.relative(path)}: {message}")

    def document(self, path: Path, expected_type: type) -> object | None:
        value = self.documents.get(path)
        if value is None:
            self.error(path, "required file is missing or could not be parsed")
            return None
        if not isinstance(value, expected_type):
            self.error(path, f"expected top-level {expected_type.__name__}")
            return None
        return value

    def record_map(self, path: Path) -> dict[str, dict]:
        records = self.document(path, list)
        if records is None:
            return {}

        result: dict[str, dict] = {}
        seen: Counter[str] = Counter()
        for offset, record in enumerate(records, start=1):
            if not isinstance(record, dict):
                self.error(path, f"record #{offset} must be an object")
                continue
            record_id = record.get("id")
            if not isinstance(record_id, str) or not record_id:
                self.error(path, f"record #{offset} has no non-empty string id")
                continue
            seen[record_id] += 1
            result.setdefault(record_id, record)

        for record_id, count in sorted(seen.items()):
            if count > 1:
                self.error(path, f"duplicate id '{record_id}' appears {count} times")
        return result

    def check_list_refs(
        self,
        path: Path,
        record_id: str,
        record: dict,
        field: str,
        valid_ids: set[str],
        *,
        required: bool = True,
        allow_duplicates: bool = False,
    ) -> list[str]:
        value = record.get(field)
        if value is None and not required:
            return []
        if not isinstance(value, list):
            self.error(path, f"{record_id}.{field} must be an array")
            return []

        result: list[str] = []
        for item in value:
            if not isinstance(item, str):
                self.error(path, f"{record_id}.{field} contains a non-string value {item!r}")
                continue
            result.append(item)
            if item not in valid_ids:
                self.error(path, f"{record_id}.{field} references unknown id '{item}'")

        duplicates = sorted(item for item, count in Counter(result).items() if count > 1)
        if duplicates and not allow_duplicates:
            self.error(path, f"{record_id}.{field} contains duplicate ids: {', '.join(duplicates)}")
        return result

    def check_scalar_ref(
        self,
        path: Path,
        record_id: str,
        record: dict,
        field: str,
        valid_ids: set[str],
        *,
        required: bool = True,
    ) -> str | None:
        value = record.get(field)
        if value is None and not required:
            return None
        if not isinstance(value, str) or not value:
            self.error(path, f"{record_id}.{field} must be a non-empty string id")
            return None
        if value not in valid_ids:
            self.error(path, f"{record_id}.{field} references unknown id '{value}'")
        return value

    def check_integer(
        self,
        path: Path,
        record_id: str,
        record: dict,
        field: str,
        *,
        minimum: int = 0,
    ) -> None:
        value = record.get(field)
        if isinstance(value, bool) or not isinstance(value, int):
            self.error(path, f"{record_id}.{field} must be an integer")
        elif value < minimum:
            self.error(path, f"{record_id}.{field} must be at least {minimum}, got {value}")

    def validate_theme(self, theme: Path) -> None:
        index_path = theme / "index.json"
        index = self.document(index_path, dict)
        if index is None:
            return

        data_root = theme / "json" / "gamedata"
        paths = {name: data_root / f"{name}.json" for name in {
            *INDEX_TABLES.values(), "stones", "images", "sounds", "musics", "fonts"
        }}
        tables = {name: self.record_map(path) for name, path in paths.items()}

        for index_key, table_name in INDEX_TABLES.items():
            declared = index.get(index_key)
            if not isinstance(declared, list):
                self.error(index_path, f"{index_key} must be an array")
                continue
            if not declared or declared[0] != "none":
                self.error(index_path, f"{index_key} must begin with the reserved 'none' id")
            if any(not isinstance(item, str) or not item for item in declared):
                self.error(index_path, f"{index_key} must contain only non-empty string ids")
                continue
            duplicates = sorted(item for item, count in Counter(declared).items() if count > 1)
            if duplicates:
                self.error(index_path, f"{index_key} contains duplicate ids: {', '.join(duplicates)}")

            declared_ids = set(declared[1:])
            record_ids = set(tables[table_name])
            missing = sorted(declared_ids - record_ids)
            unknown = sorted(record_ids - declared_ids)
            if missing:
                self.error(index_path, f"{index_key} has ids without {table_name}.json records: {', '.join(missing)}")
            if unknown:
                self.error(paths[table_name], f"records are absent from {index_key}: {', '.join(unknown)}")

        ids = {name: set(records) for name, records in tables.items()}
        clan_ids = ids["clans"] | {"none"}

        tooltips = index.get("tooltips")
        if not isinstance(tooltips, dict):
            self.error(index_path, "tooltips must be an object")
        elif tooltips.get("font") not in ids["fonts"]:
            self.error(index_path, f"tooltips.font references unknown font '{tooltips.get('font')}'")

        resource_files = {
            path.name.lower()
            for path in theme.rglob("*")
            if path.is_file()
        }
        for table_name in ("images", "sounds", "musics", "fonts"):
            path = paths[table_name]
            for record_id, record in tables[table_name].items():
                for field in ("file", "file:ru"):
                    filename = record.get(field)
                    if field != "file" and filename is None:
                        continue
                    if not isinstance(filename, str) or not filename:
                        self.error(path, f"{record_id}.{field} must be a non-empty filename")
                    elif Path(filename).name.lower() not in resource_files:
                        self.error(path, f"{record_id}.{field} resource '{filename}' does not exist in the theme")

        avatar_path = paths["avatars"]
        spell_owners: defaultdict[str, set[str]] = defaultdict(set)
        creature_owners: defaultdict[str, set[str]] = defaultdict(set)
        for avatar_id, avatar in tables["avatars"].items():
            self.check_scalar_ref(avatar_path, avatar_id, avatar, "image", ids["images"])
            self.check_scalar_ref(avatar_path, avatar_id, avatar, "portrait", ids["images"])
            clans = self.check_list_refs(avatar_path, avatar_id, avatar, "clans", ids["clans"])
            spells = self.check_list_refs(avatar_path, avatar_id, avatar, "spells", ids["spells"])
            creatures = self.check_list_refs(avatar_path, avatar_id, avatar, "creatures", ids["creatures"])
            ability = avatar.get("ability")
            if ability is not None and ability not in ids["abilities"]:
                self.error(avatar_path, f"{avatar_id}.ability references unknown id '{ability}'")
            profile = avatar.get("ai_profile")
            if profile not in AI_PROFILES:
                self.error(avatar_path, f"{avatar_id}.ai_profile has unsupported value '{profile}'")

            if avatar_id == "random":
                if spells or creatures:
                    self.error(avatar_path, "random avatar must not own spells or creatures")
            elif not clans or not spells or not creatures:
                self.error(avatar_path, f"playable avatar '{avatar_id}' must have clans, spells, and creatures")

            for spell_id in spells:
                spell_owners[spell_id].add(avatar_id)
            for creature_id in creatures:
                creature_owners[creature_id].add(avatar_id)

        creature_path = paths["creatures"]
        cast_grantors: defaultdict[str, list[str]] = defaultdict(list)
        for creature_id, creature in tables["creatures"].items():
            for field in ("attack", "defense", "loyalty", "ranger", "cost"):
                self.check_integer(creature_path, creature_id, creature, field)
            self.check_integer(creature_path, creature_id, creature, "move", minimum=1)
            self.check_scalar_ref(creature_path, creature_id, creature, "image1", ids["images"])
            self.check_scalar_ref(creature_path, creature_id, creature, "image2", ids["images"])
            self.check_scalar_ref(creature_path, creature_id, creature, "sound1", ids["sounds"])
            self.check_list_refs(
                creature_path,
                creature_id,
                creature,
                "stones",
                ids["stones"],
                allow_duplicates=True,
            )
            specials = self.check_list_refs(creature_path, creature_id, creature, "specials", ids["specials"])
            if not isinstance(creature.get("unique"), bool):
                self.error(creature_path, f"{creature_id}.unique must be true or false")
            if "fly" in creature and not isinstance(creature["fly"], bool):
                self.error(creature_path, f"{creature_id}.fly must be true or false")
            for speciality in specials:
                if speciality.startswith("cast_"):
                    cast_grantors[speciality].append(creature_id)

        spell_path = paths["spells"]
        for spell_id, spell in tables["spells"].items():
            self.check_integer(spell_path, spell_id, spell, "cost")
            self.check_scalar_ref(
                spell_path,
                spell_id,
                spell,
                "image",
                ids["images"],
                required=False,
            )
            self.check_scalar_ref(spell_path, spell_id, spell, "sound", ids["sounds"], required=False)
            self.check_list_refs(
                spell_path,
                spell_id,
                spell,
                "stones",
                ids["stones"],
                allow_duplicates=True,
            )
            if spell_owners.get(spell_id) and not spell.get("image"):
                self.error(spell_path, f"avatar-owned spell '{spell_id}' must define an image")
            target = spell.get("target")
            if target not in SPELL_TARGETS:
                self.error(spell_path, f"{spell_id}.target has unsupported value '{target}'")

            if "persistent" in spell and not isinstance(spell["persistent"], bool):
                self.error(spell_path, f"{spell_id}.persistent must be true or false")
            if "duration" in spell:
                self.check_integer(spell_path, spell_id, spell, "duration", minimum=1)
            if "effect" in spell:
                effect = spell["effect"]
                if not isinstance(effect, list) or len(effect) not in (0, 4):
                    self.error(spell_path, f"{spell_id}.effect must be empty or [attack, ranged, defense, loyalty]")
                elif any(isinstance(value, bool) or not isinstance(value, int) for value in effect):
                    self.error(spell_path, f"{spell_id}.effect must contain only integers")

        stone_path = paths["stones"]
        for stone_id, stone in tables["stones"].items():
            for field in ("image1", "image2", "image3"):
                self.check_scalar_ref(stone_path, stone_id, stone, field, ids["images"])

        wind_path = paths["winds"]
        for wind_id, wind in tables["winds"].items():
            self.check_scalar_ref(wind_path, wind_id, wind, "image", ids["images"])

        clan_path = paths["clans"]
        for clan_id, clan in tables["clans"].items():
            for field in ("button", "flag1", "flag2", "image", "town", "townflag1", "townflag2"):
                self.check_scalar_ref(clan_path, clan_id, clan, field, ids["images"])

        land_path = paths["lands"]
        for land_id, land in tables["lands"].items():
            clan = land.get("clan")
            if clan not in clan_ids:
                self.error(land_path, f"{land_id}.clan references unknown id '{clan}'")
            borders = self.check_list_refs(land_path, land_id, land, "borders", ids["lands"])
            if land_id in borders:
                self.error(land_path, f"{land_id}.borders must not reference itself")
            for border in borders:
                reverse = tables["lands"].get(border, {}).get("borders", [])
                if isinstance(reverse, list) and land_id not in reverse:
                    self.error(land_path, f"asymmetric border: {land_id} -> {border}, but reverse link is missing")

        indexed_cast_specials = {special for special in ids["specials"] if special.startswith("cast_")}
        missing_mappings = sorted(indexed_cast_specials - set(CAST_SPELLS))
        stale_mappings = sorted(set(CAST_SPELLS) - indexed_cast_specials)
        if missing_mappings:
            self.error(paths["specials"], f"cast specialities lack spell mappings: {', '.join(missing_mappings)}")
        if stale_mappings:
            self.error(paths["specials"], f"spell mappings reference unknown cast specialities: {', '.join(stale_mappings)}")
        for speciality, spell_id in CAST_SPELLS.items():
            if spell_id not in ids["spells"]:
                self.error(paths["specials"], f"{speciality} maps to missing spell '{spell_id}'")
            grantors = cast_grantors.get(speciality, [])
            if len(grantors) != 1:
                joined = ", ".join(grantors) if grantors else "none"
                self.error(creature_path, f"{speciality} must be granted by one unique creature; found {joined}")
            elif not tables["creatures"][grantors[0]].get("unique"):
                self.error(creature_path, f"{speciality} grantor '{grantors[0]}' must be a unique creature")

        if theme.name == "default":
            intro_path = theme / "json" / "screen_intro.json"
            intro = self.document(intro_path, dict)
            if intro is not None:
                frames = intro.get("frames")
                if not isinstance(frames, list) or len(frames) != 17:
                    count = len(frames) if isinstance(frames, list) else "not an array"
                    self.error(intro_path, f"original intro must contain 17 frames, got {count}")
                else:
                    for offset, frame in enumerate(frames, start=1):
                        if not isinstance(frame, dict):
                            self.error(intro_path, f"intro frame #{offset} must be an object")
                            continue

                        expected_image = f"introframe_{offset}.png"
                        image = frame.get("image")
                        actual_image = image.get("file") if isinstance(image, dict) else None
                        if actual_image != expected_image:
                            self.error(
                                intro_path,
                                f"intro frame #{offset} must use '{expected_image}', got {actual_image!r}",
                            )

                        for language, expected_sound_file in (
                            ("en", f"intro_en_{offset:02d}.ogg"),
                            ("ru", f"intro_ru_{offset:02d}.ogg"),
                        ):
                            expected_sound = f"intro_{language}_{offset:02d}"
                            actual_sound = frame.get(f"sound:{language}")
                            if actual_sound != expected_sound:
                                self.error(
                                    intro_path,
                                    f"intro frame #{offset} must use sound '{expected_sound}', got {actual_sound!r}",
                                )
                            sound_file = tables["sounds"].get(expected_sound, {}).get("file")
                            if sound_file != expected_sound_file:
                                self.error(
                                    paths["sounds"],
                                    f"{expected_sound} must map to '{expected_sound_file}', got {sound_file!r}",
                                )

                        duration = frame.get("duration_ms")
                        if isinstance(duration, bool) or not isinstance(duration, int) or duration < 1000:
                            self.error(
                                intro_path,
                                f"intro frame #{offset}.duration_ms must be an integer of at least 1000",
                            )

            localized_sounds = {
                record_id: record.get("file:ru")
                for record_id, record in tables["sounds"].items()
                if record.get("file:ru") is not None
            }
            expected_localized_sounds = {
                record_id: f"voice_ru_{resource_id}.ogg"
                for record_id, resource_id in RUSSIAN_SOUND_RESOURCE_IDS.items()
            }
            if localized_sounds != expected_localized_sounds:
                missing = sorted(set(expected_localized_sounds) - set(localized_sounds))
                stale = sorted(set(localized_sounds) - set(expected_localized_sounds))
                wrong = sorted(
                    record_id for record_id in set(localized_sounds) & set(expected_localized_sounds)
                    if localized_sounds[record_id] != expected_localized_sounds[record_id]
                )
                details = []
                if missing:
                    details.append(f"missing: {', '.join(missing)}")
                if stale:
                    details.append(f"unexpected: {', '.join(stale)}")
                if wrong:
                    details.append(f"wrong file: {', '.join(wrong)}")
                self.error(paths["sounds"], "Russian localized sound contract differs (" + "; ".join(details) + ")")

            for clan_id, expected in CANONICAL_CLAN_NAMES.items():
                actual = tables["clans"].get(clan_id, {}).get("name")
                if actual != expected:
                    self.error(clan_path, f"canonical clan id '{clan_id}' must be named '{expected}', got {actual!r}")
            for creature_id, expected in CANONICAL_CREATURE_PRICES.items():
                actual = tables["creatures"].get(creature_id, {}).get("cost")
                if actual != expected:
                    self.error(creature_path, f"canonical price {creature_id}.cost must be {expected}, got {actual}")
            for spell_id, expected in CANONICAL_SPELL_PRICES.items():
                actual = tables["spells"].get(spell_id, {}).get("cost")
                if actual != expected:
                    self.error(spell_path, f"canonical price {spell_id}.cost must be {expected}, got {actual}")
            for spell_id, expected in CANONICAL_SPELL_OWNERS.items():
                actual = spell_owners.get(spell_id, set())
                if actual != expected:
                    self.error(avatar_path, f"canonical owners of spell '{spell_id}' must be {sorted(expected)}, got {sorted(actual)}")
            for creature_id, expected in CANONICAL_CREATURE_OWNERS.items():
                actual = creature_owners.get(creature_id, set())
                if actual != expected:
                    self.error(avatar_path, f"canonical owners of creature '{creature_id}' must be {sorted(expected)}, got {sorted(actual)}")
            if tables["avatars"].get("ziag", {}).get("ability") != "monacle":
                self.error(avatar_path, "Ziag must retain the documented Monocle ability")


def main() -> int:
    repository = Path(__file__).resolve().parents[1]
    json_files = sorted((repository / "themes").rglob("*.json"))
    if not json_files:
        print("No theme JSON files found.", file=sys.stderr)
        return 1

    documents: dict[Path, object] = {}
    syntax_errors: list[str] = []
    for path in json_files:
        try:
            with path.open("r", encoding="utf-8") as source:
                documents[path] = json.load(source)
        except (OSError, UnicodeError, json.JSONDecodeError) as error:
            syntax_errors.append(f"{path.relative_to(repository).as_posix()}: {error}")

    validator = ThemeValidator(repository, documents)
    theme_roots = sorted(path.parent for path in json_files if path.name == "index.json")
    if not theme_roots:
        syntax_errors.append("themes: no theme index.json files found")
    for theme in theme_roots:
        validator.validate_theme(theme)

    errors = syntax_errors + validator.errors
    if errors:
        for error in errors:
            print(error, file=sys.stderr)
        print(f"Theme JSON validation failed with {len(errors)} error(s).", file=sys.stderr)
        return 1

    print(f"Validated {len(json_files)} theme JSON files and semantic contracts for {len(theme_roots)} theme(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
