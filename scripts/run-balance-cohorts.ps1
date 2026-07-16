param(
    [ValidateRange(1, 8)]
    [int]$SeedCount = 1,
    [string]$OutputDirectory = "diagnostics\balance-cohorts",
    [ValidateSet("Easy", "Normal", "Hard")]
    [string]$Difficulty = "Normal",
    [ValidateSet("Balanced", "Aggressive", "Economic", "Control")]
    [string]$BehaviorProfile = "Balanced",
    [ValidateSet("Red", "Yellow", "Aqua", "Purple")]
    [string[]]$Clans = @("Red", "Yellow", "Aqua", "Purple"),
    [switch]$KeepEngineLog,
    [switch]$KeepMatchRecords
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$lab = Join-Path $PSScriptRoot "run-balance-lab.ps1"
$outputPath = if ([IO.Path]::IsPathRooted($OutputDirectory)) {
    [IO.Path]::GetFullPath($OutputDirectory)
} else {
    [IO.Path]::GetFullPath((Join-Path $repoRoot $OutputDirectory))
}
$utf8 = New-Object System.Text.UTF8Encoding($false)
$invariant = [Globalization.CultureInfo]::InvariantCulture

$expectedScenariosPath = [IO.Path]::GetFullPath((Join-Path $outputPath "scenarios"))
$outputPrefix = $outputPath.TrimEnd([IO.Path]::DirectorySeparatorChar,
    [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
if (-not $expectedScenariosPath.StartsWith($outputPrefix,
        [StringComparison]::OrdinalIgnoreCase)) {
    throw "Refusing to clean an unexpected cohort scenarios path."
}
$scenariosPath = $expectedScenariosPath
if (Test-Path -LiteralPath $scenariosPath) {
    Remove-Item -LiteralPath $scenariosPath -Recurse -Force
}
New-Item -ItemType Directory -Path $scenariosPath -Force | Out-Null

$baseline = [ordered]@{
    red = "nucrus"
    yellow = "lakkho"
    aqua = "ziag"
    purple = "dayla"
}
$candidates = [ordered]@{
    red = @("nucrus", "orachi", "javed", "logun")
    yellow = @("lakkho", "niana", "javed", "logun")
    aqua = @("ziag", "niana", "kierac", "logun")
    purple = @("dayla", "orachi", "kierac", "logun")
}
$slotIndexes = @{ red = 0; yellow = 1; aqua = 2; purple = 3 }
$selectedClans = @($Clans | ForEach-Object { $_.ToLowerInvariant() } |
    Select-Object -Unique)
if (-not $selectedClans.Count) {
    throw "At least one clan cohort is required."
}

function Format-Invariant($Value) {
    if ($null -eq $Value) { return "" }
    return ([double]$Value).ToString("0.######", $invariant)
}

function Find-AvatarSummary($Report, [string]$Avatar) {
    return $Report.avatar_summary | Where-Object {
        $_.avatar -eq $Avatar
    } | Select-Object -First 1
}

function New-Roster([string]$Clan, [string]$Candidate) {
    $roster = @($baseline.red, $baseline.yellow, $baseline.aqua, $baseline.purple)
    $roster[$slotIndexes[$Clan]] = $Candidate
    return $roster
}

function Invoke-CohortScenario([string]$Id, [string[]]$Roster) {
    $scenarioDirectory = Join-Path $scenariosPath $Id
    Write-Host "===== Balance cohort scenario: $Id ($($Roster -join ',')) ====="
    $arguments = @{
        SeedCount = $SeedCount
        OutputDirectory = $scenarioDirectory
        Difficulty = $Difficulty
        BehaviorProfile = $BehaviorProfile
        Roster = $Roster
    }
    if ($KeepEngineLog) { $arguments.KeepEngineLog = $true }
    if ($KeepMatchRecords) { $arguments.KeepMatchRecords = $true }
    & $lab @arguments

    $reportPath = Join-Path $scenarioDirectory "balance-report.json"
    if (-not (Test-Path -LiteralPath $reportPath -PathType Leaf)) {
        throw "Cohort scenario '$Id' did not produce balance-report.json."
    }
    $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
    $requestedRoster = $Roster -join ","
    if ($report.schema_version -ne 2 -or
        $report.difficulty -ne $Difficulty.ToLowerInvariant() -or
        $report.behavior_profile -ne $BehaviorProfile.ToLowerInvariant() -or
        (@($report.roster) -join ",") -ne $requestedRoster -or
        [int]$report.legal_clan_assignments -ne 1 -or
        [int]$report.planned_matches -ne ($SeedCount * 4) -or
        [int]$report.completed_matches -ne [int]$report.planned_matches) {
        throw "Cohort scenario '$Id' report does not match its controlled contract."
    }
    return [PSCustomObject]@{
        id = $Id
        roster = $Roster
        report = $report
    }
}

$baselineRoster = New-Roster "red" $baseline.red
$scenarioData = @(Invoke-CohortScenario "baseline" $baselineRoster)
foreach ($clan in $selectedClans) {
    foreach ($candidate in $candidates[$clan]) {
        if ($candidate -eq $baseline[$clan]) { continue }
        $scenarioData += Invoke-CohortScenario "$clan-$candidate" `
            (New-Roster $clan $candidate)
    }
}

$baselineScenario = $scenarioData | Where-Object { $_.id -eq "baseline" } |
    Select-Object -First 1
$cells = @()
$csvRows = @()
foreach ($clan in $selectedClans) {
    $baselineAvatar = $baseline[$clan]
    $baselineSummary = Find-AvatarSummary $baselineScenario.report $baselineAvatar
    if ($null -eq $baselineSummary) {
        throw "Baseline report is missing avatar '$baselineAvatar'."
    }

    foreach ($candidate in $candidates[$clan]) {
        $scenario = if ($candidate -eq $baselineAvatar) {
            $baselineScenario
        } else {
            $scenarioData | Where-Object { $_.id -eq "$clan-$candidate" } |
                Select-Object -First 1
        }
        $summary = Find-AvatarSummary $scenario.report $candidate
        if ($null -eq $scenario -or $null -eq $summary) {
            throw "Cohort '$clan/$candidate' is missing its scenario or avatar summary."
        }

        $delta = [ordered]@{
            rank_one_rate = [double]$summary.rank_one_rate - [double]$baselineSummary.rank_one_rate
            mean_final_rank = [double]$summary.mean_final_rank - [double]$baselineSummary.mean_final_rank
            mean_total_score = [double]$summary.mean_total_score - [double]$baselineSummary.mean_total_score
            mean_summons = [double]$summary.mean_summons - [double]$baselineSummary.mean_summons
            mean_spells_cast = [double]$summary.mean_spells_cast - [double]$baselineSummary.mean_spells_cast
            mean_final_units = [double]$summary.mean_final_units - [double]$baselineSummary.mean_final_units
            territory = [double]$summary.mean_final_categories.territory -
                [double]$baselineSummary.mean_final_categories.territory
            summon_circles = [double]$summary.mean_final_categories.summon_circles -
                [double]$baselineSummary.mean_final_categories.summon_circles
            units = [double]$summary.mean_final_categories.units -
                [double]$baselineSummary.mean_final_categories.units
            spell_points = [double]$summary.mean_final_categories.spell_points -
                [double]$baselineSummary.mean_final_categories.spell_points
            land_claims = [double]$summary.mean_final_categories.land_claims -
                [double]$baselineSummary.mean_final_categories.land_claims
        }
        $cell = [ordered]@{
            clan = $clan
            baseline_avatar = $baselineAvatar
            candidate_avatar = $candidate
            scenario = $scenario.id
            report = "scenarios/$($scenario.id)/balance-report.json"
            roster = @($scenario.roster)
            scheduled = [int]$summary.scheduled
            completed = [int]$summary.completed
            rank_one_finishes = [int]$summary.rank_one_finishes
            rank_one_rate = [double]$summary.rank_one_rate
            rank_one_rate_low_95 = [double]$summary.rank_one_rate_low_95
            rank_one_rate_high_95 = [double]$summary.rank_one_rate_high_95
            mean_final_rank = [double]$summary.mean_final_rank
            mean_total_score = [double]$summary.mean_total_score
            mean_summons = [double]$summary.mean_summons
            mean_spells_cast = [double]$summary.mean_spells_cast
            mean_final_units = [double]$summary.mean_final_units
            mean_final_categories = $summary.mean_final_categories
            delta_vs_baseline_avatar_same_clan = $delta
        }
        $cells += [PSCustomObject]$cell
        $csvRows += [PSCustomObject][ordered]@{
            clan = $clan
            baseline_avatar = $baselineAvatar
            candidate_avatar = $candidate
            scenario = $scenario.id
            completed = [int]$summary.completed
            rank_one_finishes = [int]$summary.rank_one_finishes
            rank_one_rate = Format-Invariant $summary.rank_one_rate
            rank_one_rate_low_95 = Format-Invariant $summary.rank_one_rate_low_95
            rank_one_rate_high_95 = Format-Invariant $summary.rank_one_rate_high_95
            mean_final_rank = Format-Invariant $summary.mean_final_rank
            mean_total_score = Format-Invariant $summary.mean_total_score
            mean_summons = Format-Invariant $summary.mean_summons
            mean_spells_cast = Format-Invariant $summary.mean_spells_cast
            mean_final_units = Format-Invariant $summary.mean_final_units
            rank_one_delta_vs_baseline = Format-Invariant $delta.rank_one_rate
            rank_delta_vs_baseline = Format-Invariant $delta.mean_final_rank
            total_delta_vs_baseline = Format-Invariant $delta.mean_total_score
            territory_delta_vs_baseline = Format-Invariant $delta.territory
            units_delta_vs_baseline = Format-Invariant $delta.units
            spell_points_delta_vs_baseline = Format-Invariant $delta.spell_points
            land_claims_delta_vs_baseline = Format-Invariant $delta.land_claims
        }
    }
}

$gameRevision = (git -c safe.directory=$($repoRoot.Replace('\', '/')) rev-parse HEAD).Trim()
$engineRevision = (git -C (Join-Path $repoRoot "engine") `
    -c safe.directory=$((Join-Path $repoRoot "engine").Replace('\', '/')) rev-parse HEAD).Trim()
$gameDirty = [bool](git -c safe.directory=$($repoRoot.Replace('\', '/')) status --porcelain)
$engineDirty = [bool](git -C (Join-Path $repoRoot "engine") `
    -c safe.directory=$((Join-Path $repoRoot "engine").Replace('\', '/')) status --porcelain)

$reportRoot = [ordered]@{
    schema_version = 1
    game_revision = $gameRevision
    game_dirty = $gameDirty
    engine_revision = $engineRevision
    engine_dirty = $engineDirty
    seed_count = $SeedCount
    seeds = @($baselineScenario.report.seeds)
    difficulty = $Difficulty.ToLowerInvariant()
    behavior_profile = $BehaviorProfile.ToLowerInvariant()
    selected_clans = $selectedClans
    baseline_roster = @($baselineRoster)
    scenario_count = $scenarioData.Count
    planned_matches = [int](($scenarioData | ForEach-Object {
        [int]$_.report.planned_matches
    } | Measure-Object -Sum).Sum)
    completed_matches = [int](($scenarioData | ForEach-Object {
        [int]$_.report.completed_matches
    } | Measure-Object -Sum).Sum)
    comparison_contract = "candidate avatar minus baseline avatar in the same fixed clan slot; other three avatars, doctrine, difficulty, seeds and seat rotations remain fixed"
    cells = $cells
}
if ($reportRoot.completed_matches -ne $reportRoot.planned_matches) {
    throw "Controlled cohort aggregation contains incomplete matches."
}

$jsonPath = Join-Path $outputPath "balance-cohorts.json"
$csvPath = Join-Path $outputPath "balance-cohorts.csv"
$textPath = Join-Path $outputPath "balance-cohorts.txt"
[IO.File]::WriteAllText($jsonPath, ($reportRoot | ConvertTo-Json -Depth 12), $utf8)
[IO.File]::WriteAllLines($csvPath, @($csvRows | ConvertTo-Csv -NoTypeInformation), $utf8)

$text = New-Object Text.StringBuilder
[void]$text.AppendLine("Four Winds Reborn controlled avatar/clan cohorts")
[void]$text.AppendLine("Seeds: $SeedCount; difficulty: $($Difficulty.ToLowerInvariant()); profile: $($BehaviorProfile.ToLowerInvariant())")
[void]$text.AppendLine("Scenarios: $($scenarioData.Count); isolated matches: $($reportRoot.planned_matches)")
[void]$text.AppendLine("Delta is candidate minus the baseline avatar in the same clan; lower mean-rank delta is better.")
[void]$text.AppendLine()
[void]$text.AppendLine("Clan   Baseline  Candidate  Done  Rank1    Mean rank  Mean total  dRank1   dRank   dTotal")
foreach ($row in $csvRows) {
    [void]$text.AppendLine(("{0,-7} {1,-9} {2,-10} {3,4}  {4,6}  {5,9}  {6,10}  {7,7}  {8,6}  {9,7}" -f `
        $row.clan, $row.baseline_avatar, $row.candidate_avatar, $row.completed, `
        $row.rank_one_rate, $row.mean_final_rank, $row.mean_total_score, `
        $row.rank_one_delta_vs_baseline, $row.rank_delta_vs_baseline, `
        $row.total_delta_vs_baseline))
}
[IO.File]::WriteAllText($textPath, $text.ToString(), $utf8)

Write-Host $text.ToString()
Write-Host "Controlled cohort reports: $outputPath"
