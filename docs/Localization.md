# Localization and Voice Assets

Four Winds Reborn ships built-in English text and selectable Russian gettext
catalogs for both bundled content packages. Leaving Settings applies a language
change without restarting the process. The per-user `settings.json` stores the
selection as `language: en` or `language: ru`.

Localization is package-scoped where presentation or lore differs:

- **Classic** preserves the original English presentation and recovered
  Russian terminology, button art, narration and voice resources.
- **Reborn** supplies its own bilingual names, descriptions, narration,
  announcer and hero voice sets while retaining the same gameplay contracts.

## Maintained text sources

Each package owns a complete gettext set:

- `themes/<package>/lang/runewars-na.pot` — messages extracted from current
  code and package data;
- `themes/<package>/lang/ru.po` — reviewed Russian source catalog;
- `themes/<package>/lang/ru.mo` — compiled runtime catalog.

Package JSON contains the data-driven names, descriptions and Encyclopedia
articles. `scripts/apply-russian-translations.py` remains the reproducible
curated terminology source for Classic. Reborn translations are maintained
with its package catalog and data.

Theme validation requires an exact Russian runtime translation for every
non-empty wizard, creature, spell, speciality and ability description,
including significant tabs, line breaks and colour markup. This prevents a
catalog entry that appears translated in source form from silently falling
back to English at runtime.

## Classic provenance

Classic Russian terminology was reconciled against the localized `RWTEXT.DLL`
from the owner's original Rune War CD and installed Windows 98 copy. Both
copies have the same hash. The DLL stores Russian text as ASCII-like code
points rendered through the original custom font; extraction was decoded into
Unicode and used as a terminology reference rather than retained as a runtime
dependency.

The localized `RWSOUND.DLL` from the CD and installed copy also match.
Resource-by-resource comparison with the English distribution identified the
authentic Russian avatar, creature, clan, round and wind calls. Classic maps
those files through the same logical sound IDs as English.

The seventeen longer resources (`4048` through `4064`) are the original story
narration and correspond one-to-one with `introframe_1.png` through
`introframe_17.png`.

## Localized images

Text-bearing sprites may select a localized source with `file:ru`. Both
packages provide English and Russian button atlases for Mahjong calls and
shared controls such as Pass, Chow, Pung, Kong, Game, Close, Cancel, Done,
Dismiss, Undo, Info, Chat, Menu, Start, Previous and Next. Reborn-specific
controls use matching localized artwork.

Sprite cache keys include the selected source file, so switching language
updates image controls without restarting.

## Voices and narration

Logical sound IDs stay stable across languages. Package sound tables select
their English and Russian media through `file:en` and `file:ru` when the two
tracks differ. This covers narrator calls, hero names, creature names, winds,
round flow and hero-specific Mahjong calls without coupling gameplay code to a
particular filename or voice cast.

Intro screens select narration with `sound:en` and `sound:ru`. Reborn also
stores `duration_ms:en` and `duration_ms:ru` per frame because its bilingual
performances intentionally have different pacing. Classic uses the authentic
shared frame timing. Esc, Enter, Space or click skips the intro cleanly.

Voice-generation scripts and approved narration text are development inputs;
release packages contain the reviewed OGG assets, not service credentials or
temporary synthesis output.

## Contributor workflow

When changing localized content:

1. Edit the package's source PO and JSON data, not only the generated MO.
2. Rebuild the MO catalog and keep English/Russian logical IDs aligned.
3. If narration changes, update the corresponding per-language frame duration.
4. Run `python scripts/validate-json.py`.
5. Smoke-test an in-process English/Russian switch, text-bearing buttons,
   creature and hero announcements, Mahjong calls and all intro frames.

Do not copy Classic narrative or media into Reborn as a silent fallback.
Intentional shared package-neutral assets must be declared in
`themes/reborn/provenance.json`. A release-ready package may explicitly retain
generic non-voice compatibility effects, but unclassified inherited media and
all inherited narrative remain release blockers. Standalone status additionally
requires the retained compatibility list to be empty.
