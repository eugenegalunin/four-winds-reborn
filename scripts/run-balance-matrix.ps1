param(
    [ValidateRange(1, 8)]
    [int]$SeedCount = 1,
    [string]$OutputDirectory = "diagnostics\balance-matrix",
    [ValidateSet("Native", "Balanced", "Aggressive", "Economic", "Control")]
    [string[]]$Profiles = @("Native", "Balanced", "Aggressive", "Economic", "Control"),
    [ValidateSet("Training", "Easy", "Normal", "Hard", "Unfair")]
    [string[]]$Difficulties = @("Easy", "Normal", "Hard"),
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
$scenariosPath = Join-Path $outputPath "scenarios"
$utf8 = New-Object System.Text.UTF8Encoding($false)
$invariant = [Globalization.CultureInfo]::InvariantCulture

New-Item -ItemType Directory -Path $scenariosPath -Force | Out-Null

function Find-Scenario($Scenarios, [string]$Difficulty, [string]$Profile) {
    return $Scenarios | Where-Object {
        $_.difficulty -eq $Difficulty -and $_.behavior_profile -eq $Profile
    } | Select-Object -First 1
}

function Find-AvatarSummary($Scenario, [string]$Avatar) {
    if ($null -eq $Scenario) { return $null }
    return $Scenario.report.avatar_summary | Where-Object {
        $_.avatar -eq $Avatar
    } | Select-Object -First 1
}

function New-Delta($Current, $Reference) {
    if ($null -eq $Reference) { return $null }
    return [ordered]@{
        rank_one_rate = [double]$Current.rank_one_rate - [double]$Reference.rank_one_rate
        mean_final_rank = [double]$Current.mean_final_rank - [double]$Reference.mean_final_rank
        mean_total_score = [double]$Current.mean_total_score - [double]$Reference.mean_total_score
        mean_summons = [double]$Current.mean_summons - [double]$Reference.mean_summons
        mean_spells_cast = [double]$Current.mean_spells_cast - [double]$Reference.mean_spells_cast
    }
}

function Format-Invariant($Value) {
    if ($null -eq $Value) { return "" }
    return ([double]$Value).ToString("0.######", $invariant)
}

$selectedProfiles = @($Profiles | ForEach-Object { $_.ToLowerInvariant() } | Select-Object -Unique)
$selectedDifficulties = @($Difficulties | ForEach-Object {
    $_.ToLowerInvariant()
} | Select-Object -Unique)
if (-not $selectedProfiles.Count -or -not $selectedDifficulties.Count) {
    throw "The balance matrix requires at least one profile and one difficulty."
}
$scenarioData = @()

foreach ($difficulty in $selectedDifficulties) {
    foreach ($profile in $selectedProfiles) {
        $scenarioId = "$difficulty-$profile"
        $scenarioDirectory = Join-Path $scenariosPath $scenarioId
        Write-Host "===== Balance scenario: $scenarioId ====="
        $arguments = @{
            SeedCount = $SeedCount
            OutputDirectory = $scenarioDirectory
            Difficulty = $difficulty
            BehaviorProfile = $profile
        }
        if ($KeepEngineLog) { $arguments.KeepEngineLog = $true }
        if ($KeepMatchRecords) { $arguments.KeepMatchRecords = $true }
        & $lab @arguments

        $reportPath = Join-Path $scenarioDirectory "balance-report.json"
        if (-not (Test-Path -LiteralPath $reportPath)) {
            throw "Scenario '$scenarioId' did not produce balance-report.json."
        }
        $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
        if ($report.schema_version -ne 2 -or $report.difficulty -ne $difficulty -or
            $report.behavior_profile -ne $profile) {
            throw "Scenario '$scenarioId' report configuration does not match its matrix cell."
        }
        $scenarioData += [PSCustomObject]@{
            id = $scenarioId
            difficulty = $difficulty
            behavior_profile = $profile
            report = $report
        }
    }
}

$matrixScenarios = @()
$csvRows = @()
foreach ($scenario in $scenarioData) {
    $completedMatches = @($scenario.report.matches | Where-Object { $_.status -eq "complete" })
    $meanTicks = if ($completedMatches.Count) {
        [double](($completedMatches | Measure-Object -Property ticks -Average).Average)
    } else { 0.0 }
    $avatarRows = @()
    foreach ($summary in $scenario.report.avatar_summary) {
        $nativeReference = Find-AvatarSummary `
            (Find-Scenario $scenarioData $scenario.difficulty "native") $summary.avatar
        $normalReference = Find-AvatarSummary `
            (Find-Scenario $scenarioData "normal" $scenario.behavior_profile) $summary.avatar
        $profileDelta = New-Delta $summary $nativeReference
        $difficultyDelta = New-Delta $summary $normalReference
        $avatarRow = [ordered]@{
            avatar = $summary.avatar
            scheduled = [int]$summary.scheduled
            completed = [int]$summary.completed
            rank_one_rate = [double]$summary.rank_one_rate
            rank_one_rate_low_95 = [double]$summary.rank_one_rate_low_95
            rank_one_rate_high_95 = [double]$summary.rank_one_rate_high_95
            mean_final_rank = [double]$summary.mean_final_rank
            mean_total_score = [double]$summary.mean_total_score
            mean_summons = [double]$summary.mean_summons
            mean_spells_cast = [double]$summary.mean_spells_cast
            delta_vs_native_same_difficulty = $profileDelta
            delta_vs_normal_same_profile = $difficultyDelta
        }
        $avatarRows += [PSCustomObject]$avatarRow
        $csvRows += [PSCustomObject][ordered]@{
            scenario = $scenario.id
            difficulty = $scenario.difficulty
            behavior_profile = $scenario.behavior_profile
            avatar = $summary.avatar
            completed = [int]$summary.completed
            rank_one_rate = Format-Invariant $summary.rank_one_rate
            rank_one_rate_low_95 = Format-Invariant $summary.rank_one_rate_low_95
            rank_one_rate_high_95 = Format-Invariant $summary.rank_one_rate_high_95
            mean_final_rank = Format-Invariant $summary.mean_final_rank
            mean_total_score = Format-Invariant $summary.mean_total_score
            mean_summons = Format-Invariant $summary.mean_summons
            mean_spells_cast = Format-Invariant $summary.mean_spells_cast
            mean_match_ticks = Format-Invariant $meanTicks
            rank_one_delta_vs_native_same_difficulty = Format-Invariant `
                $(if ($null -ne $profileDelta) { $profileDelta.rank_one_rate } else { $null })
            rank_delta_vs_native_same_difficulty = Format-Invariant `
                $(if ($null -ne $profileDelta) { $profileDelta.mean_final_rank } else { $null })
            rank_one_delta_vs_normal_same_profile = Format-Invariant `
                $(if ($null -ne $difficultyDelta) { $difficultyDelta.rank_one_rate } else { $null })
            rank_delta_vs_normal_same_profile = Format-Invariant `
                $(if ($null -ne $difficultyDelta) { $difficultyDelta.mean_final_rank } else { $null })
        }
    }

    $matrixScenarios += [PSCustomObject][ordered]@{
        id = $scenario.id
        difficulty = $scenario.difficulty
        behavior_profile = $scenario.behavior_profile
        report = "scenarios/$($scenario.id)/balance-report.json"
        planned_matches = [int]$scenario.report.planned_matches
        completed_matches = [int]$scenario.report.completed_matches
        mean_match_ticks = $meanTicks
        avatars = $avatarRows
    }
}

$matrix = [ordered]@{
    schema_version = 1
    seed_count = $SeedCount
    seeds = @($scenarioData[0].report.seeds)
    axes = [ordered]@{
        difficulties = $selectedDifficulties
        behavior_profiles = $selectedProfiles
    }
    comparison_contract = [ordered]@{
        profile_delta = "same difficulty minus native behavior at the same difficulty"
        difficulty_delta = "same behavior profile minus normal difficulty for that profile"
    }
    scenarios = $matrixScenarios
}

$jsonPath = Join-Path $outputPath "balance-matrix.json"
$csvPath = Join-Path $outputPath "balance-matrix.csv"
$textPath = Join-Path $outputPath "balance-matrix.txt"
[IO.File]::WriteAllText($jsonPath, ($matrix | ConvertTo-Json -Depth 12), $utf8)
[IO.File]::WriteAllLines($csvPath, @($csvRows | ConvertTo-Csv -NoTypeInformation), $utf8)

$text = New-Object Text.StringBuilder
[void]$text.AppendLine("Four Winds Reborn balance comparison matrix")
[void]$text.AppendLine("Seeds: $SeedCount; scenarios: $($matrixScenarios.Count); matches per scenario: $($scenarioData[0].report.planned_matches)")
[void]$text.AppendLine("Profile delta uses native at the same difficulty; difficulty delta uses normal for the same profile.")
[void]$text.AppendLine()
[void]$text.AppendLine("Scenario             Avatar    Done  Rank1    Mean rank  Mean total  dRank1/profile  dRank/difficulty")
foreach ($row in $csvRows) {
    [void]$text.AppendLine(("{0,-20} {1,-9} {2,4}  {3,6}  {4,9}  {5,10}  {6,14}  {7,16}" -f `
        $row.scenario, $row.avatar, $row.completed, $row.rank_one_rate, `
        $row.mean_final_rank, $row.mean_total_score, `
        $row.rank_one_delta_vs_native_same_difficulty, `
        $row.rank_delta_vs_normal_same_profile))
}
[IO.File]::WriteAllText($textPath, $text.ToString(), $utf8)

Write-Host $text.ToString()
Write-Host "Balance matrix reports: $outputPath"
