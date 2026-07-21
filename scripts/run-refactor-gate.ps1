param(
    [ValidateSet("Quick", "Full")]
    [string]$Mode = "Quick",
    [string]$OutputDirectory = "diagnostics\refactor-gate",
    [string]$BaselineDirectory = "",
    [string]$BaselineManifest = "tests\fixtures\balance-cohorts-15.6e-refactor.sha256.json",
    [ValidateRange(1, 8)]
    [int]$Jobs = 2,
    [switch]$Resume,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$outputPath = if ([IO.Path]::IsPathRooted($OutputDirectory)) {
    [IO.Path]::GetFullPath($OutputDirectory)
} else {
    [IO.Path]::GetFullPath((Join-Path $repoRoot $OutputDirectory))
}

if (-not $SkipBuild) {
    & (Join-Path $PSScriptRoot "build-windows.ps1") -DebugBuild
}
$verificationPrefix = if ($SkipBuild) {
    "prebuilt binary"
} else {
    "build/tests"
}

if ($Mode -eq "Quick") {
    if ($Resume) {
        Write-Warning "-Resume has no effect in Quick mode; the four-match canary is rerun."
    }
    $quickOutput = Join-Path $outputPath "quick"
    & (Join-Path $PSScriptRoot "run-balance-lab.ps1") `
        -SeedCount 1 `
        -OutputDirectory $quickOutput `
        -Difficulty Normal `
        -BehaviorProfile Balanced `
        -Roster Nucrus,Lakkho,Ziag,Dayla `
        -Jobs $Jobs
    $compareArguments = @{
        CandidateDirectory = $quickOutput
    }
    if ($BaselineDirectory) {
        $compareArguments.BaselineDirectory = Join-Path $BaselineDirectory `
            "scenarios\baseline"
    } else {
        $compareArguments.BaselineManifest = $BaselineManifest
        $compareArguments.BaselinePrefix = "scenarios/baseline"
    }
    & (Join-Path $PSScriptRoot "compare-balance-cohorts.ps1") @compareArguments
    Write-Host "Refactor gate PASS (Quick): $verificationPrefix and four-match baseline are unchanged."
    return
}

$fullOutput = Join-Path $outputPath "full"
$arguments = @{
    SeedCount = 1
    OutputDirectory = $fullOutput
    Difficulty = "Normal"
    BehaviorProfile = "Balanced"
    Clans = @("Red", "Yellow", "Aqua", "Purple")
    Jobs = $Jobs
}
if ($BaselineDirectory) {
    $arguments.BaselineDirectory = $BaselineDirectory
} else {
    $arguments.BaselineManifest = $BaselineManifest
}
if ($Resume) { $arguments.Resume = $true }
& (Join-Path $PSScriptRoot "run-balance-cohorts.ps1") @arguments
Write-Host "Refactor gate PASS (Full): $verificationPrefix and all 52 controlled matches are unchanged."
