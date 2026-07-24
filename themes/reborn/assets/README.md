# Reborn runtime assets

The Reborn theme keeps its runtime media under semantic directories:

- `images` - interface art, portraits, maps and sprite atlases;
- `audio` - sound effects, speech and intro narration in OGG format;
- `music` - the package-specific OGG soundtrack;
- `fonts` - fonts bundled specifically for this theme.

Theme JSON refers to resources by unique file name. The resource loader scans
the complete theme tree recursively, so these directories describe ownership
and purpose without leaking filesystem paths into gameplay data.

Editable and generated source material lives outside the runtime package under
the local ignored `art/` and `voicepacks/` workspaces. Build helpers compile
reviewed runtime copies into this directory. Fresh clones and player packages
need only `themes/reborn`; the high-resolution generation sources are not
runtime dependencies.

The July 2026 release audit removed unused Classic media, replaced the legacy
UI atlases and redirected announcer fallbacks to the package-specific English
voice set. A small, explicit compatibility layer of generic non-voice gameplay
effects remains; see `../provenance.json`.
Pixel-level image and decoded-PCM audio comparisons found no additional
re-encoded Classic copies outside that declared layer.

The compatibility layer is deliberately still present because every file is
referenced at runtime. It consists of generic effects `1000`-`1006`,
`1010`-`1013`, `1021`-`1026` and `1030`-`1037`. Do not prune these by filename
alone.

The five post-fork difficulty portraits are package-neutral project art shared
by Classic and Reborn. `provenance.json` declares their exact paths, and the
validator accepts them only while both copies exist and remain byte-identical.
