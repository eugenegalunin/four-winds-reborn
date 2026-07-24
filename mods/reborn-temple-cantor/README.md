# Reborn: Temple Cantor

This optional Russian voice mod preserves the first original synthetic narrator
created for the Reborn theme: a solemn temple cantor with a measured ritual
cadence.

The pack contains:

- 17 Russian intro scenes;
- 18 Russian game-announcer lines;
- 29 Russian creature announcements;
- the exact localized intro timings used by this recording;
- the reusable source voice reference and its metadata.

No TTS model or Python environment is required to use the rendered voice pack.

## Install

Close the game and run:

```powershell
.\apply.ps1 -GameRoot "C:\Games\four-winds-reborn"
```

When the mod is kept directly inside a source checkout, `GameRoot` can be
omitted. The installer backs up every replaced OGG and `screen_intro.json`
under `backups/reborn-temple-cantor-<timestamp>` before changing the Reborn
theme.

This directory is also the forward-compatible content package for the planned
in-game mod manager. Until that loader is implemented, `apply.ps1` performs the
same theme-overlay operation explicitly and reversibly.

## Restore

Copy the contents of the backup's `theme` directory back over
`themes/reborn`. The backup path is printed after installation.

## Voice source

`source/reference.wav` is the selected `temple_cantor` identity generated for
the project. It is included so the voice can be regenerated or extended later
without depending on the current default Reborn narrator.
