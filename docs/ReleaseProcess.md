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

Choose the next calendar tag in the existing `vYYYYMMDD.N` format. After all
platform checks and owner smoke testing are accepted, reconcile the complete
integration branch into `main` as one explicit release merge commit:

```bash
git switch main
git pull --ff-only origin main
git merge --no-ff develop -m "release: vYYYYMMDD.N"
git push origin main
git tag -a vYYYYMMDD.N -m "Four Winds Reborn vYYYYMMDD.N"
git push origin vYYYYMMDD.N
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
git merge --no-ff main -m "reconcile hotfix vYYYYMMDD.N"
git push origin develop
```

Generated builds, diagnostics, dumps, local handoff notes and credentials must
never be staged in either branch.
