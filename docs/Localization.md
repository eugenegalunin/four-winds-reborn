# Localization and original voice assets

Four Winds Reborn ships built-in English strings and a selectable Russian
gettext catalog. Language changes are applied after leaving Settings and do not
require restarting the process. The selection is stored in the per-user
`settings.json` as `language: en` or `language: ru`.

## Russian terminology source

The Russian catalog was reconciled against the localized `RWTEXT.DLL` from the
owner's original Rune War CD and its installed Windows 98 copy. Both copies have
the same hash. That DLL stores Russian text as ASCII-like code points rendered
through the original custom font; the extraction was decoded into Unicode and
used as terminology reference rather than copied as a runtime dependency.

The maintained sources are:

- `themes/default/lang/runewars-na.pot` — messages extracted from current code;
- `themes/default/lang/ru.po` — reviewed Russian source catalog;
- `themes/default/lang/ru.mo` — compiled runtime catalog;
- `scripts/apply-russian-translations.py` — reproducible curated terminology.

Current menus, saves/recovery, gameplay controls, names, scores, creatures and
spells are covered. The Encyclopedia stores its preserved archive articles in
reviewed English and Russian form, while biographies and descriptions are read
from the installed theme and translated through gettext. Theme validation now
requires an exact Russian runtime translation for every non-empty wizard,
creature, spell, speciality and ability description, including its original
tabs, line breaks and colour markup. This prevents a catalog entry that looks
translated in source form from silently falling back to English in the game.

## Original localized button art

The Russian `BUTTONS` bitmap from the localized `RWIMAGE.DLL` is preserved as
`buttons_ru.png`. Text-bearing button sprites select it through the same
explicit `file:ru` contract used by localized audio, while English continues
to use `buttons.png`. This covers the original Russian artwork for Mahjong
calls and shared controls such as Pass, Chow, Pung, Kong, Game, Close, Cancel,
Done, Dismiss, Undo, Info, Chat, Menu, Start, Previous and Next. `OK` remains
language-neutral, matching the localized original. The Reborn-only Order
control has a matching Russian `Приказ` sprite derived from the original
button frame. Sprite cache keys include the selected source file, so changing
language in Settings updates image controls without restarting the process.

## Voice assets

The localized `RWSOUND.DLL` from the CD and installed copy also match. Comparing
it resource-by-resource with the owner's English distribution proved that 61
short WAVE resources differ. Fifty-seven are identified runtime calls: avatar
names and winning calls, all 29 creature names, clan names, round start/winds
and Logun's localized Kong. The other Mahjong calls and ordinary UI/combat
effects are intentionally identical in both editions. The theme keeps the
existing English OGGs and supplies an explicit `file:ru` for every proven
localized call, so changing language switches voices without changing logical
sound IDs. Four differing but still unidentified resources remain unmapped.

The seventeen longer resources (`4048` through `4064`) are now proven to be the
story narration: they correspond one-to-one with the already
tracked `introframe_1.png` through `introframe_17.png`. The owner's English
distribution supplied the matching authentic English resources under the same
IDs. Every English/Russian pair has exactly the same duration, confirming the
shared frame timing. The v0.1.0 startup intro plays the selected narration once
before the main menu, with timed fades, progress indication and
Esc/Enter/Space/click skip.
