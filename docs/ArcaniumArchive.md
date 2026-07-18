# Arcanium Rune War Web Archive

This document records the official Arcanium Productions Rune War website as
captured by the Internet Archive around 2001. It is the provenance ledger for
the in-game Encyclopedia. The archive is treated as a historical primary
source, not as an authority that silently overrides current Four Winds Reborn
rules or balance values.

Primary snapshot:
<https://web.archive.org/web/20010304082845/http://www.arcanium.com/Games/RuneWar/index.html>

## Preservation model

The original site used a 750x550 frameset and split its content into Tales,
Denizens, Wizards, Oracle, Scry, Herald and Main. Reborn preserves it in three
layers:

1. `docs/Backgrnd.txt`, `docs/Chars.txt` and `docs/Manual.txt` contain the
   surviving original long-form manuscripts, biographies, rules and unit
   descriptions.
2. `themes/default/json/gamedata/*.json` contains the playable wizard,
   creature, spell and ability records. The Encyclopedia reads these live so
   displayed statistics always match the installed theme.
3. `themes/default/json/encyclopedia.json` preserves bilingual, player-facing
   articles for the archive-only framing, clan pages, FAQ, gallery and Herald
   material. It labels historical behaviour and continuity differences instead
   of presenting them as current Reborn rules.

This arrangement deliberately avoids embedding obsolete HTML, navigation GIFs,
email addresses and dead download links in the game UI. If the Wayback snapshot
disappears, the factual and narrative corpus still remains in the repository.

## Content inventory

### Main

- `Menu/Menu.shtml` — Jutka the Wandering Prophet introduces the approaching
  Rune War, the Oracle of Jaar, foreign wizards, the twenty-five-year stakes
  and an uncertain contest.
- `Menu/Frame2.html` — navigation for the seven main sections.
- `Frame4.html` — header frame; it played the Marz MIDI theme.

### Tales

- `Tales/Introduction.shtml` — the seventeenth Rune War in year 3416.
- `Tales/MakingRuneWar.shtml` — negotiations that created the Rune War system.
- `Tales/ClanMarz.shtml` — amphibious refugees, culture and motives of Marz.
- `Tales/ClanMaitha.shtml` — native wind-magic culture and ambitions of Maitha.
- `Tales/ClanKartha.shtml` — Karthan society, honour and sword tradition.
- `Tales/ClanIz.shtml` — Iz history, adaptability and tactical culture.
- `Tales/FourWindsBackground.shtml` — Isle of Four Winds background.
- `Tales/BirthOfTower.shtml` — origin of the Tower and its four mages.

The website's 2991/3016/3416 chronology differs from dates in another surviving
Arcanium background manuscript. Both are preserved as separate historical
continuities; neither is silently rewritten.

### Denizens

- `Denizens/SkullCreatures.shtml` — Skeleton Horde through Maz'Ra.
- `Denizens/SwordCreatures.shtml` — Knight Templar through King Drago.
- `Denizens/MagicNumbers.shtml` — Sand Wraith through Shanahan.
- `Denizens/Elementals.shtml` — Fire, Earth, Air and Water Elementals.
- `Denizens/ThreeDragons.shtml` — White, Green and Red Dragons.
- `Denizens/SpecialCreatures.shtml` — Juggernaut, Tornado, Griffon and Chameleon.

The site contains several old typos and values from the classic release. The
Encyclopedia keeps its six-part taxonomy but obtains names, statistics, costs,
specialities and descriptions from current theme data.

### Wizards

- `Wizards/Orachi.shtml`
- `Wizards/Lakkho.shtml`
- `Wizards/Dayla.shtml`
- `Wizards/Ziag.shtml`
- `Wizards/Niana.shtml`
- `Wizards/Kierac.shtml`
- `Wizards/Logun.shtml`
- `Wizards/Nucrus.shtml`
- `Wizards/Javed.shtml`

The complete biographies survive in `docs/Chars.txt` and avatar theme data.
Reborn additionally displays each wizard's current clans, AI strategy and
special ability.

### Oracle FAQ

- `Oracle/Installation.shtml` — Windows 95, DirectX 5, 1024x768x256,
  `dplayx.dll`, disk space and virtual-memory advice. Museum-only.
- `Oracle/MahJong.shtml` — winds, turn indicator, timer, map view and tutorial.
- `Oracle/SpellCasting.shtml` — spell points, runes, summoning, targets,
  healing and magic resistance.
- `Oracle/MapScreen.shtml` — flags, movement, dismiss, undo, claims and the
  classic combat sequence.

Original controls and timer behaviour are explicitly labelled historical when
they differ from Reborn.

### Scry gallery

- `Scry/MapScreen.shtml` — `Scry/img/MapScreen2.JPG`
- `Scry/CombatScreen.shtml` — `Scry/img/CombatScreen2.JPG`
- `Scry/WellScreen.shtml` — `Scry/img/WellScreen2.JPG`
- `Scry/ScoreScreen.shtml` — `Scry/img/ScoreScreen2.JPG`
- `Scry/VictoryScreen.shtml` — `Scry/img/VictoryScreen2.JPG`

The gallery composition is preserved in the Legacy article. The corresponding
classic screens and art are already represented by the original default theme;
current README screenshots document the restored build itself.

### Herald

- `Herald/Herald2.shtml` — contemporary notices from Strategy Plus, Next
  Generation, GameSpot, ZENtropy and AllAboutGames.
- `Herald/Herald3.shtml` — comments attributed to PC Gamer UK, Games.exe,
  TEN's File Factory and beta testers.

These are retained as historical reception, never presented as modern reviews.

## Archive URL pattern

For any relative path above, prepend:

`https://web.archive.org/web/20010304082845/http://www.arcanium.com/Games/RuneWar/`

Some individual pages were captured on later 2001–2002 timestamps. Wayback
normally redirects the primary snapshot URL to the closest available capture.

