# Current controlled avatar tiers

Status: provisional controlled tier list, 2026-07-16.

This is the current strength view for automated full matches. It supersedes the
old `--balance-only` ordering as a tier list. That fast deterministic report is
still useful for catching data drift, impossible plans and extreme value
changes, but it does not play complete games and must not be read as the current
competitive ranking.

## Evidence contract

The source is controlled cohort 1 documented in
[`BalanceLab.md`](BalanceLab.md#controlled-avatarclan-cohort-1): 416 complete
isolated matches, eight fixed seeds, four seat rotations, `Normal` difficulty
and one common `Balanced` doctrine. Every clan/avatar cell contains 32 fixtures.
Sixteen retained outlier replays reproduce every authoritative hash.

Three outcomes receive equal weight:

- rank-one rate, higher is better;
- mean final rank, lower is better;
- mean total score, higher is better.

The direct clan order standardizes those three outcomes among the four legal
candidates in the same fixed clan slot. The overall list fits each outcome as
`clan slot + avatar`, predicts every avatar in the average clan slot, standardizes
the three adjusted predictions and averages them, with mean rank inverted. This
removes the first-order clan/map-position effect without pretending that every
avatar was legal in every clan.

## Direct fixed-clan tiers

These are the primary, directly observed tiers. `~` means the outcomes are mixed
or close enough that the current cohort does not justify separating the pair.

| Fixed clan | Current order | Reading |
| --- | --- | --- |
| Maitha | **S Nucrus** > **A Orachi** > **B Logun** > **D Javed** | Nucrus leads all three outcomes; Javed trails all three. |
| Kartha | **S Niana ~ Lakkho** > **A Logun** > **B Javed** | Niana leads wins/rank, Lakkho leads total; there is no runaway winner. |
| Iz | **S Kierac** >> **B Logun ~ Niana** > **D Ziag** | Kierac leads all three outcomes; Ziag trails all three. |
| Marz | **S Orachi** > **A Logun** > **B Dayla ~ Kierac** | Logun wins more often, but Orachi has the best rank and total; Dayla/Kierac split the remaining metrics. |

## Provisional clan-adjusted overall tiers

The composite score is measured in standardized units; zero is the nine-avatar
average. `Clan coverage` is the number of different fixed clan slots directly
observed for that avatar, not a statistical confidence percentage.

| Tier | Avatar | Composite | Adjusted rank one | Adjusted mean rank | Adjusted mean total | Clan coverage |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| S | Nucrus | +1.948 | 42.06% | 1.792 | 15.408 | 1 |
| A | Orachi | +0.691 | 33.20% | 2.160 | 14.551 | 2 |
| A | Kierac | +0.598 | 32.81% | 2.168 | 14.422 | 2 |
| B | Logun | -0.079 | 28.91% | 2.383 | 13.922 | 4 |
| B | Lakkho | -0.137 | 29.82% | 2.591 | 14.186 | 1 |
| C | Dayla | -0.259 | 18.10% | 2.372 | 14.507 | 1 |
| C | Niana | -0.297 | 27.73% | 2.527 | 13.918 | 2 |
| D | Javed | -0.949 | 20.31% | 2.645 | 13.563 | 2 |
| D | Ziag | -1.517 | 19.40% | 2.839 | 12.962 | 1 |

The three additive fits explain 81.2% of cell-level rank-one variation, 91.5%
of mean-rank variation and 93.0% of mean-total variation. There are only four
residual degrees of freedom, so the exact decimals are reproducibility details,
not a claim of precision.

## Interpretation

Nucrus is still the current S-tier signal, but the controlled evidence says
something narrower than the old synthetic ranking: he is dominant in the tested
Maitha slot, while his average-clan estimate is bridged through shared candidates
rather than directly observed in four clans. Orachi and Kierac form a credible
A tier instead of leaving Nucrus alone above an undifferentiated field.

Logun is the best cross-clan anchor because he was observed in all four slots.
Ziag remains the weakest adjusted avatar and the weakest direct Iz candidate,
although his adjusted rank-one estimate is higher than the raw Iz result; that
is evidence that both Ziag and the Iz slot contribute to the old collapse.

Do not balance against the tier letters alone. The next decision should compare
paired Ziag/Kierac fixtures and representative replays, then change one reviewed
passive, creature, spell or scoring family at a time and rerun the same cohort.
