param(
    [Parameter(Mandatory = $true)]
    [string]$BaselineReport,
    [Parameter(Mandatory = $true)]
    [string]$CandidateReport,
    [Parameter(Mandatory = $true)]
    [string]$BaselineAvatar,
    [Parameter(Mandatory = $true)]
    [string]$CandidateAvatar,
    [string]$OutputDirectory = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$utf8 = New-Object System.Text.UTF8Encoding($false)
$invariant = [Globalization.CultureInfo]::InvariantCulture
$BaselineAvatar = $BaselineAvatar.ToLowerInvariant()
$CandidateAvatar = $CandidateAvatar.ToLowerInvariant()

function Resolve-ReportPath([string]$Path) {
    $resolved = if ([IO.Path]::IsPathRooted($Path)) {
        [IO.Path]::GetFullPath($Path)
    } else {
        [IO.Path]::GetFullPath((Join-Path $repoRoot $Path))
    }
    if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
        throw "Balance report does not exist: $resolved"
    }
    return $resolved
}

function Format-Number($Value) {
    return ([double]$Value).ToString("0.###", $invariant)
}

function Get-Mean([object[]]$Values) {
    if (-not $Values.Count) { return 0.0 }
    return [double](($Values | Measure-Object -Average).Average)
}

function Get-FixtureKey($Match) {
    return "$([int]$Match.seed_index)|$([int]$Match.seat_rotation)|" +
        "$([int]$Match.clan_assignment)|$([int]$Match.run)"
}

function Find-Player($Match, [string]$Avatar) {
    return $Match.players | Where-Object { $_.avatar -eq $Avatar } |
        Select-Object -First 1
}

function Find-Telemetry($Match, [string]$Avatar) {
    return $Match.player_telemetry | Where-Object { $_.avatar -eq $Avatar } |
        Select-Object -First 1
}

function Find-StagePlayer($Match, [string]$Stage, [string]$Avatar) {
    $stageData = $Match.stages | Where-Object { $_.stage -eq $Stage } |
        Select-Object -First 1
    if ($null -eq $stageData) { return $null }
    return $stageData.players | Where-Object { $_.avatar -eq $Avatar } |
        Select-Object -First 1
}

function Get-OutcomeCounts([object[]]$Rows, [string]$Property,
                           [bool]$LowerIsBetter) {
    $better = 0
    $same = 0
    $worse = 0
    foreach ($row in $Rows) {
        $value = [double]$row.$Property
        if ($value -eq 0) { ++$same }
        elseif (($LowerIsBetter -and $value -lt 0) -or
                (-not $LowerIsBetter -and $value -gt 0)) { ++$better }
        else { ++$worse }
    }
    return [PSCustomObject][ordered]@{
        better = $better
        same = $same
        worse = $worse
    }
}

function Get-SubjectTotals([object[]]$Telemetry, [string]$Property) {
    $totals = @{}
    foreach ($player in $Telemetry) {
        foreach ($item in @($player.$Property)) {
            if (-not $totals.ContainsKey($item.name)) { $totals[$item.name] = 0 }
            $totals[$item.name] += [int]$item.count
        }
    }
    return $totals
}

function New-SubjectComparison($BaselineTotals, $CandidateTotals) {
    $names = @($BaselineTotals.Keys) + @($CandidateTotals.Keys) |
        Sort-Object -Unique
    return @($names | ForEach-Object {
        $baseline = if ($BaselineTotals.ContainsKey($_)) {
            [int]$BaselineTotals[$_]
        } else { 0 }
        $candidate = if ($CandidateTotals.ContainsKey($_)) {
            [int]$CandidateTotals[$_]
        } else { 0 }
        [PSCustomObject][ordered]@{
            name = $_
            baseline_total = $baseline
            candidate_total = $candidate
            delta = $candidate - $baseline
        }
    } | Sort-Object @{ Expression = { [Math]::Abs($_.delta) }; Descending = $true },
        name)
}

$baselinePath = Resolve-ReportPath $BaselineReport
$candidatePath = Resolve-ReportPath $CandidateReport
$baseline = Get-Content -LiteralPath $baselinePath -Raw | ConvertFrom-Json
$candidate = Get-Content -LiteralPath $candidatePath -Raw | ConvertFrom-Json

foreach ($entry in @(
    [PSCustomObject]@{ Name = "baseline"; Report = $baseline },
    [PSCustomObject]@{ Name = "candidate"; Report = $candidate }
)) {
    if ([int]$entry.Report.schema_version -ne 2 -or
        [int]$entry.Report.completed_matches -ne [int]$entry.Report.planned_matches -or
        @($entry.Report.matches).Count -ne [int]$entry.Report.completed_matches -or
        @($entry.Report.matches | Where-Object { $_.status -ne "complete" }).Count) {
        throw "$($entry.Name) report is incomplete or does not use balance schema 2."
    }
}
if ($baseline.difficulty -ne $candidate.difficulty -or
    $baseline.behavior_profile -ne $candidate.behavior_profile -or
    (@($baseline.seeds) -join ",") -ne (@($candidate.seeds) -join ",")) {
    throw "Reports must use the same difficulty, behavior profile and seed list."
}
$baselineRoster = [string[]]@($baseline.roster)
$candidateRoster = [string[]]@($candidate.roster)
$baselineAvatarIndex = [Array]::IndexOf($baselineRoster, $BaselineAvatar)
$candidateAvatarIndex = [Array]::IndexOf($candidateRoster, $CandidateAvatar)
if ($baselineAvatarIndex -lt 0 -or $candidateAvatarIndex -lt 0 -or
    $baselineAvatarIndex -ne $candidateAvatarIndex) {
    throw "Target avatars must occupy the same roster slot."
}
$startingWinds = @("east", "south", "west", "north")

$candidateByFixture = @{}
foreach ($match in $candidate.matches) {
    $key = Get-FixtureKey $match
    if ($candidateByFixture.ContainsKey($key)) {
        throw "Candidate report contains duplicate fixture '$key'."
    }
    $candidateByFixture[$key] = $match
}

$pairs = @()
$baselineTelemetry = @()
$candidateTelemetry = @()
foreach ($baselineMatch in $baseline.matches) {
    $key = Get-FixtureKey $baselineMatch
    if (-not $candidateByFixture.ContainsKey($key)) {
        throw "Candidate report is missing fixture '$key'."
    }
    $candidateMatch = $candidateByFixture[$key]
    $baselinePlayer = Find-Player $baselineMatch $BaselineAvatar
    $candidatePlayer = Find-Player $candidateMatch $CandidateAvatar
    $baselinePlayerTelemetry = Find-Telemetry $baselineMatch $BaselineAvatar
    $candidatePlayerTelemetry = Find-Telemetry $candidateMatch $CandidateAvatar
    if ($null -eq $baselinePlayer -or $null -eq $candidatePlayer -or
        $null -eq $baselinePlayerTelemetry -or $null -eq $candidatePlayerTelemetry) {
        throw "Fixture '$key' is missing target player or telemetry data."
    }
    if ($baselinePlayer.clan -ne $candidatePlayer.clan) {
        throw "Fixture '$key' does not keep the target clan fixed."
    }

    $baselineOpponents = @($baselineMatch.players | Where-Object {
        $_.avatar -ne $BaselineAvatar
    } | ForEach-Object { $_.avatar } | Sort-Object)
    $candidateOpponents = @($candidateMatch.players | Where-Object {
        $_.avatar -ne $CandidateAvatar
    } | ForEach-Object { $_.avatar } | Sort-Object)
    if (($baselineOpponents -join ",") -ne ($candidateOpponents -join ",")) {
        throw "Fixture '$key' changes more than the target avatar."
    }

    $baselineTelemetry += $baselinePlayerTelemetry
    $candidateTelemetry += $candidatePlayerTelemetry
    $pairs += [PSCustomObject][ordered]@{
        fixture = $key
        seed_index = [int]$baselineMatch.seed_index
        seed = [uint32]$baselineMatch.seed
        seat_rotation = [int]$baselineMatch.seat_rotation
        clan_assignment = [int]$baselineMatch.clan_assignment
        run = [int]$baselineMatch.run
        starting_wind = $startingWinds[($baselineAvatarIndex +
            [int]$baselineMatch.seat_rotation) % $startingWinds.Count]
        baseline_rank = [int]$baselinePlayer.final_rank
        candidate_rank = [int]$candidatePlayer.final_rank
        rank_delta = [int]$candidatePlayer.final_rank - [int]$baselinePlayer.final_rank
        baseline_total = [int]$baselinePlayer.total_score
        candidate_total = [int]$candidatePlayer.total_score
        total_delta = [int]$candidatePlayer.total_score - [int]$baselinePlayer.total_score
        baseline_hash = $baselineMatch.final_state_hash
        candidate_hash = $candidateMatch.final_state_hash
    }
}
if ($pairs.Count -ne $candidateByFixture.Count) {
    throw "Reports do not contain the same number of fixtures."
}

function New-GroupSummary([string]$Name, [object[]]$Rows) {
    return [PSCustomObject][ordered]@{
        name = $Name
        fixtures = $Rows.Count
        baseline_rank_one = @($Rows | Where-Object { $_.baseline_rank -eq 1 }).Count
        candidate_rank_one = @($Rows | Where-Object { $_.candidate_rank -eq 1 }).Count
        rank = Get-OutcomeCounts $Rows "rank_delta" $true
        mean_rank_delta = Get-Mean @($Rows | ForEach-Object { $_.rank_delta })
        total = Get-OutcomeCounts $Rows "total_delta" $false
        mean_total_delta = Get-Mean @($Rows | ForEach-Object { $_.total_delta })
        min_total_delta = [int](($Rows | Measure-Object total_delta -Minimum).Minimum)
        max_total_delta = [int](($Rows | Measure-Object total_delta -Maximum).Maximum)
    }
}

$overall = New-GroupSummary "overall" $pairs
$byWind = @($pairs | Group-Object starting_wind | Sort-Object Name | ForEach-Object {
    New-GroupSummary $_.Name @($_.Group)
})
$bySeed = @($pairs | Group-Object seed | Sort-Object Name | ForEach-Object {
    New-GroupSummary $_.Name @($_.Group)
})

$stages = @()
foreach ($stage in @("early", "middle", "late")) {
    $rows = @()
    foreach ($pair in $pairs) {
        $baselineMatch = $baseline.matches | Where-Object {
            (Get-FixtureKey $_) -eq $pair.fixture
        } | Select-Object -First 1
        $candidateMatch = $candidateByFixture[$pair.fixture]
        $baselineStage = Find-StagePlayer $baselineMatch $stage $BaselineAvatar
        $candidateStage = Find-StagePlayer $candidateMatch $stage $CandidateAvatar
        if ($null -eq $baselineStage -or $null -eq $candidateStage) {
            throw "Fixture '$($pair.fixture)' is missing the '$stage' stage."
        }
        $rows += [PSCustomObject]@{
            rank_delta = [int]$candidateStage.final_rank - [int]$baselineStage.final_rank
            total_delta = [int]$candidateStage.total_score - [int]$baselineStage.total_score
            baseline_rank = [int]$baselineStage.final_rank
            candidate_rank = [int]$candidateStage.final_rank
        }
    }
    $stages += New-GroupSummary $stage $rows
}

$categories = @()
foreach ($category in @("territory", "summon_circles", "units",
                        "spell_points", "land_claims")) {
    $rawDelta = @()
    $standingDelta = @()
    $rankDelta = @()
    foreach ($pair in $pairs) {
        $baselineMatch = $baseline.matches | Where-Object {
            (Get-FixtureKey $_) -eq $pair.fixture
        } | Select-Object -First 1
        $candidateMatch = $candidateByFixture[$pair.fixture]
        $baselinePlayer = Find-Player $baselineMatch $BaselineAvatar
        $candidatePlayer = Find-Player $candidateMatch $CandidateAvatar
        $rawDelta += [double]$candidatePlayer.categories.$category.score -
            [double]$baselinePlayer.categories.$category.score
        $standingDelta += [double]$candidatePlayer.categories.$category.standing_points -
            [double]$baselinePlayer.categories.$category.standing_points
        $rankDelta += [double]$candidatePlayer.categories.$category.rank -
            [double]$baselinePlayer.categories.$category.rank
    }
    $categories += [PSCustomObject][ordered]@{
        category = $category
        mean_raw_score_delta = Get-Mean $rawDelta
        mean_standing_points_delta = Get-Mean $standingDelta
        mean_category_rank_delta = Get-Mean $rankDelta
    }
}

$telemetry = @()
foreach ($property in @("summons", "spells_cast", "casualties", "dismissals",
                        "peak_units", "final_units")) {
    $baselineValues = @($baselineTelemetry | ForEach-Object { [double]$_.$property })
    $candidateValues = @($candidateTelemetry | ForEach-Object { [double]$_.$property })
    $baselineMean = Get-Mean $baselineValues
    $candidateMean = Get-Mean $candidateValues
    $telemetry += [PSCustomObject][ordered]@{
        metric = $property
        baseline_mean = $baselineMean
        candidate_mean = $candidateMean
        delta = $candidateMean - $baselineMean
    }
}

$summonedCreatures = New-SubjectComparison `
    (Get-SubjectTotals $baselineTelemetry "summoned_creatures") `
    (Get-SubjectTotals $candidateTelemetry "summoned_creatures")
$castSpells = New-SubjectComparison `
    (Get-SubjectTotals $baselineTelemetry "cast_spells") `
    (Get-SubjectTotals $candidateTelemetry "cast_spells")

$opponents = @()
$opponentNames = @($baseline.matches[0].players | Where-Object {
    $_.avatar -ne $BaselineAvatar
} | ForEach-Object { $_.avatar } | Sort-Object)
foreach ($avatar in $opponentNames) {
    $rankDelta = @()
    $totalDelta = @()
    foreach ($pair in $pairs) {
        $baselineMatch = $baseline.matches | Where-Object {
            (Get-FixtureKey $_) -eq $pair.fixture
        } | Select-Object -First 1
        $candidateMatch = $candidateByFixture[$pair.fixture]
        $baselinePlayer = Find-Player $baselineMatch $avatar
        $candidatePlayer = Find-Player $candidateMatch $avatar
        $rankDelta += [double]$candidatePlayer.final_rank - [double]$baselinePlayer.final_rank
        $totalDelta += [double]$candidatePlayer.total_score - [double]$baselinePlayer.total_score
    }
    $opponents += [PSCustomObject][ordered]@{
        avatar = $avatar
        mean_rank_delta = Get-Mean $rankDelta
        mean_total_delta = Get-Mean $totalDelta
    }
}

$report = [ordered]@{
    schema_version = 1
    baseline_report = $baselinePath
    candidate_report = $candidatePath
    baseline_avatar = $BaselineAvatar
    candidate_avatar = $CandidateAvatar
    clan = (Find-Player $baseline.matches[0] $BaselineAvatar).clan
    difficulty = $baseline.difficulty
    behavior_profile = $baseline.behavior_profile
    seeds = @($baseline.seeds)
    overall = $overall
    by_wind = $byWind
    by_seed = $bySeed
    stages = $stages
    categories = $categories
    telemetry = $telemetry
    summoned_creatures = $summonedCreatures
    cast_spells = $castSpells
    opponent_impact = $opponents
    pairs = $pairs
}

$outputPath = if ($OutputDirectory) {
    if ([IO.Path]::IsPathRooted($OutputDirectory)) {
        [IO.Path]::GetFullPath($OutputDirectory)
    } else {
        [IO.Path]::GetFullPath((Join-Path $repoRoot $OutputDirectory))
    }
} else {
    Join-Path (Split-Path $candidatePath -Parent) `
        "paired-$BaselineAvatar-$CandidateAvatar"
}
New-Item -ItemType Directory -Path $outputPath -Force | Out-Null

$jsonPath = Join-Path $outputPath "balance-pairs.json"
$csvPath = Join-Path $outputPath "balance-pairs.csv"
$textPath = Join-Path $outputPath "balance-pairs.txt"
[IO.File]::WriteAllText($jsonPath,
    (($report | ConvertTo-Json -Depth 12) + "`n"), $utf8)
[IO.File]::WriteAllLines($csvPath,
    @($pairs | ConvertTo-Csv -NoTypeInformation), $utf8)

$lines = @(
    "Four Winds Reborn paired balance comparison",
    "Baseline: $BaselineAvatar; candidate: $CandidateAvatar; clan: $($report.clan)",
    "Difficulty: $($report.difficulty); profile: $($report.behavior_profile)",
    "Fixtures: $($overall.fixtures)",
    "",
    "Outcome",
    "Rank one: $BaselineAvatar $($overall.baseline_rank_one), $CandidateAvatar $($overall.candidate_rank_one)",
    "Rank better/same/worse: $($overall.rank.better)/$($overall.rank.same)/$($overall.rank.worse); mean delta $(Format-Number $overall.mean_rank_delta)",
    "Total better/same/worse: $($overall.total.better)/$($overall.total.same)/$($overall.total.worse); mean delta $(Format-Number $overall.mean_total_delta); range $(Format-Number $overall.min_total_delta)..$(Format-Number $overall.max_total_delta)",
    "",
    "By wind"
)
foreach ($row in $byWind) {
    $lines += "$($row.name): rank $($row.rank.better)/$($row.rank.same)/$($row.rank.worse), dRank $(Format-Number $row.mean_rank_delta), dTotal $(Format-Number $row.mean_total_delta), candidate rank-one $($row.candidate_rank_one)"
}
$lines += ""
$lines += "Stages"
foreach ($row in $stages) {
    $lines += "$($row.name): rank $($row.rank.better)/$($row.rank.same)/$($row.rank.worse), dRank $(Format-Number $row.mean_rank_delta), dTotal $(Format-Number $row.mean_total_delta)"
}
$lines += ""
$lines += "Final categories"
foreach ($row in $categories) {
    $lines += "$($row.category): dRaw $(Format-Number $row.mean_raw_score_delta), dStanding $(Format-Number $row.mean_standing_points_delta), dRank $(Format-Number $row.mean_category_rank_delta)"
}
$lines += ""
$lines += "Telemetry"
foreach ($row in $telemetry) {
    $lines += "$($row.metric): $BaselineAvatar $(Format-Number $row.baseline_mean), $CandidateAvatar $(Format-Number $row.candidate_mean), delta $(Format-Number $row.delta)"
}
$lines += ""
$lines += "Summoned creatures (non-zero delta)"
foreach ($row in @($summonedCreatures | Where-Object { $_.delta -ne 0 })) {
    $lines += "$($row.name): $($row.baseline_total) -> $($row.candidate_total) (delta $($row.delta))"
}
$lines += ""
$lines += "Cast spells (non-zero delta)"
foreach ($row in @($castSpells | Where-Object { $_.delta -ne 0 })) {
    $lines += "$($row.name): $($row.baseline_total) -> $($row.candidate_total) (delta $($row.delta))"
}
$lines += ""
$lines += "Opponent impact (candidate scenario minus baseline)"
foreach ($row in $opponents) {
    $lines += "$($row.avatar): dRank $(Format-Number $row.mean_rank_delta), dTotal $(Format-Number $row.mean_total_delta)"
}
[IO.File]::WriteAllLines($textPath, $lines, $utf8)

$lines | ForEach-Object { Write-Host $_ }
Write-Host ""
Write-Host "Paired reports: $outputPath"
