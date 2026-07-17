# Release Process

Four Winds Reborn uses two permanent branches:

- `develop` is the integration branch. Normal gameplay, UI, data and tooling
  changes land here as focused commits.
- `main` is the published release history. It receives completed release merges
  and exceptional hotfixes; it is never used as a general work branch.

The release that introduces this policy is the one-time bootstrap: `develop`
is created from that released `main`. Every later normal release follows the
flow below.

## Normal development

Start work from current `develop`, keep related changes in a reviewable commit,
and push the integration branch:

```bash
git switch develop
git pull --ff-only origin develop
# edit, test and commit
git push origin develop
```

CI runs for pushes and pull requests targeting either permanent branch. Do not
force-push `develop` or `main` and do not rewrite their published history.

## Tagged release

Starting after the historical `v20260716.*` releases, versions use SemVer in the
`vMAJOR.MINOR.PATCH` form. The first SemVer release is `v0.1.0`. While the game
is pre-1.0, increment MINOR for a completed player-facing roadmap slice and
PATCH for compatible hotfixes. Reserve `v1.0.0` for the accepted, stable core
roadmap. The version in the tag must exactly match `project(... VERSION ...)` in
`CMakeLists.txt`.

The first SemVer release is cut only after the owner accepts the main-menu
milestone. Its release gate is:

- every enabled menu entry has a complete path with no clickable dead end;
- Continue, New Game, Load Game and Quit pass manual Windows smoke testing;
- named save create/overwrite/load/delete and emergency recovery restore pass
  manual Windows smoke testing;
- the original narrated story intro advances through all 17 frames with the
  selected English/Russian track, and Esc/Enter/Space/click each skip cleanly;
- music transitions pass manual Windows smoke testing: intro on the main menu,
  the selected clan theme in Mahjong, map music in Adventure, silence during a
  visible battle, map resume after combat, and the victory theme on summaries;
- Settings persists English/Russian, music, effects, Guardian voices, the
  Classic/Normal/Fast presentation profile and Windowed/Fullscreen display
  mode; language and display mode apply without restart;
- Classic presentation visibly deals the opening hand, animates draw/discard
  runes and slows combat, while Normal and Fast remain usable alternatives;
- entries intentionally deferred to later roadmaps remain visibly disabled;
- the menu artwork has confirmed redistribution permission or is replaced;
- local tests and all platform CI jobs are green.

The historical calendar GitHub Release records may be removed during the
`v0.1.0` cutover, after the replacement package is ready. Keep their Git tags as
history unless the owner explicitly confirms tag deletion at action time.

After all platform checks and owner smoke testing are accepted, reconcile the
complete integration branch into `main` as one explicit release merge commit:

```bash
git switch main
git pull --ff-only origin main
git merge --no-ff develop -m "release: v0.1.0"
git push origin main
git tag -a v0.1.0 -m "Four Winds Reborn v0.1.0"
git push origin v0.1.0
```

Push `main` before the tag. The release workflow rejects a tag whose commit is
not already contained in `origin/main`. An accepted tag rebuilds and tests the
game on Linux, macOS and Windows, packages all three platforms, generates
`SHA256SUMS.txt` and publishes the GitHub Release.

After publication, switch back to `develop` for the next feature commit.

## Hotfix

Create a focused hotfix from `main`, validate it on the affected platform and
merge it back as a dedicated release commit. Tag and publish it with the same
sequence as a normal release. Then reconcile released `main` back into
`develop` so the fix cannot disappear from the next release:

```bash
git switch develop
git pull --ff-only origin develop
git merge --no-ff main -m "reconcile hotfix v0.1.1"
git push origin develop
```

Generated builds, diagnostics, dumps, local handoff notes and credentials must
never be staged in either branch.
