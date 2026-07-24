param(
    [Parameter(Mandatory = $true)]
    [string]$CandidateDirectory,
    [string]$BaselineDirectory = "",
    [string]$BaselineManifest = "tests\fixtures\balance-cohorts-15.6e-refactor.sha256.json",
    [string]$BaselinePrefix = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

function Resolve-RepositoryPath([string]$Path) {
    $candidate = if ([IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $repoRoot $Path
    }
    if (-not (Test-Path -LiteralPath $candidate -PathType Container)) {
        throw "Cohort directory was not found: '$candidate'."
    }
    return (Resolve-Path -LiteralPath $candidate).Path
}

function Resolve-RepositoryFile([string]$Path) {
    $candidate = if ([IO.Path]::IsPathRooted($Path)) {
        $Path
    } else {
        Join-Path $repoRoot $Path
    }
    if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
        throw "Cohort baseline manifest was not found: '$candidate'."
    }
    return (Resolve-Path -LiteralPath $candidate).Path
}

function Get-CsvContracts([string]$Root) {
    $prefix = $Root.TrimEnd([IO.Path]::DirectorySeparatorChar,
        [IO.Path]::AltDirectorySeparatorChar) + [IO.Path]::DirectorySeparatorChar
    $contracts = @{}
    foreach ($file in @(Get-ChildItem -LiteralPath $Root -Filter "*.csv" `
            -File -Recurse | Sort-Object FullName)) {
        $relativePath = $file.FullName.Substring($prefix.Length).Replace('\', '/')
        $contracts[$relativePath] = (Get-FileHash -LiteralPath $file.FullName `
            -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    return $contracts
}

function Get-ManifestContracts([string]$Path) {
    $manifest = Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    if ([int]$manifest.schema_version -ne 1 -or
        @($manifest.csv_contracts).Count -eq 0) {
        throw "Unsupported or empty cohort baseline manifest: '$Path'."
    }
    $contracts = @{}
    foreach ($contract in @($manifest.csv_contracts)) {
        $relativePath = [string]$contract.path
        $sha256 = ([string]$contract.sha256).ToLowerInvariant()
        if (-not $relativePath -or $sha256 -notmatch '^[0-9a-f]{64}$' -or
            $contracts.ContainsKey($relativePath)) {
            throw "Invalid or duplicate CSV contract '$relativePath' in '$Path'."
        }
        $contracts[$relativePath] = $sha256
    }
    return $contracts
}

function Select-ContractPrefix($Contracts, [string]$Prefix) {
    if (-not $Prefix) { return $Contracts }
    $normalizedPrefix = $Prefix.Replace('\', '/').TrimStart('/')
    if (-not $normalizedPrefix.EndsWith('/')) {
        $normalizedPrefix += '/'
    }
    $selected = @{}
    foreach ($relativePath in $Contracts.Keys) {
        if ($relativePath.StartsWith($normalizedPrefix,
                [StringComparison]::OrdinalIgnoreCase)) {
            $selected[$relativePath.Substring($normalizedPrefix.Length)] =
                $Contracts[$relativePath]
        }
    }
    return $selected
}

$candidatePath = Resolve-RepositoryPath $CandidateDirectory
$baselineContracts = if ($BaselineDirectory) {
    Get-CsvContracts (Resolve-RepositoryPath $BaselineDirectory)
} else {
    Get-ManifestContracts (Resolve-RepositoryFile $BaselineManifest)
}
$baselineContracts = Select-ContractPrefix $baselineContracts $BaselinePrefix
$candidateContracts = Get-CsvContracts $candidatePath
if ($baselineContracts.Count -eq 0 -or $candidateContracts.Count -eq 0) {
    throw "Cohort parity requires at least one CSV contract in each directory."
}
$allPaths = @($baselineContracts.Keys + $candidateContracts.Keys |
    Sort-Object -Unique)
$mismatches = @()
foreach ($relativePath in $allPaths) {
    if (-not $baselineContracts.ContainsKey($relativePath)) {
        $mismatches += "candidate-only: $relativePath"
    } elseif (-not $candidateContracts.ContainsKey($relativePath)) {
        $mismatches += "baseline-only: $relativePath"
    } elseif ($baselineContracts[$relativePath] -cne $candidateContracts[$relativePath]) {
        $mismatches += "content: $relativePath"
    }
}

Write-Host ("Cohort parity: baseline CSV {0}; candidate CSV {1}; checked {2}; mismatches {3}" -f `
    $baselineContracts.Count, $candidateContracts.Count, $allPaths.Count,
    $mismatches.Count)
foreach ($mismatch in $mismatches) {
    Write-Host "  $mismatch"
}
if ($mismatches.Count -gt 0) {
    throw "Controlled cohort CSV contracts differ."
}
Write-Host "Cohort parity PASS: every CSV contract is byte-identical."
