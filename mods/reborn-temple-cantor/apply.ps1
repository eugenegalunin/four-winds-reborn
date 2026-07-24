[CmdletBinding()]
param(
    [string] $GameRoot
)

$ErrorActionPreference = "Stop"

if (-not $GameRoot) {
    $GameRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}

$themeRoot = Join-Path $GameRoot "themes\reborn"
$targetAudio = Join-Path $themeRoot "assets\audio"
$targetIntro = Join-Path $themeRoot "json\screen_intro.json"
$overlayAudio = Join-Path $PSScriptRoot "theme-overlay\assets\audio"
$timingFile = Join-Path $PSScriptRoot "intro-timings-ru.json"

if (-not (Test-Path -LiteralPath $targetAudio -PathType Container)) {
    throw "Reborn audio directory was not found: $targetAudio"
}

if (-not (Test-Path -LiteralPath $targetIntro -PathType Leaf)) {
    throw "Reborn intro configuration was not found: $targetIntro"
}

$audioFiles = Get-ChildItem -LiteralPath $overlayAudio -Filter "*.ogg" -File
if ($audioFiles.Count -ne 64) {
    throw "The Temple Cantor pack is incomplete: expected 64 OGG files, found $($audioFiles.Count)."
}

$stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backupRoot = Join-Path $GameRoot "backups\reborn-temple-cantor-$stamp"
$backupAudio = Join-Path $backupRoot "theme\assets\audio"
$backupIntro = Join-Path $backupRoot "theme\json\screen_intro.json"

New-Item -ItemType Directory -Path $backupAudio -Force | Out-Null
New-Item -ItemType Directory -Path (Split-Path -Parent $backupIntro) -Force | Out-Null

foreach ($source in $audioFiles) {
    $destination = Join-Path $targetAudio $source.Name
    if (Test-Path -LiteralPath $destination -PathType Leaf) {
        Copy-Item -LiteralPath $destination -Destination (Join-Path $backupAudio $source.Name)
    }
}
Copy-Item -LiteralPath $targetIntro -Destination $backupIntro

foreach ($source in $audioFiles) {
    Copy-Item -LiteralPath $source.FullName -Destination (Join-Path $targetAudio $source.Name) -Force
}

$timings = Get-Content -LiteralPath $timingFile -Raw | ConvertFrom-Json
$intro = Get-Content -LiteralPath $targetIntro -Raw | ConvertFrom-Json

if ($intro.frames.Count -ne $timings.duration_ms.Count) {
    throw "Intro timing contract mismatch: theme has $($intro.frames.Count) frames, mod has $($timings.duration_ms.Count)."
}

for ($index = 0; $index -lt $intro.frames.Count; ++$index) {
    $intro.frames[$index]."duration_ms:ru" = [int] $timings.duration_ms[$index]
}

$serializedIntro = ($intro | ConvertTo-Json -Depth 100) + [Environment]::NewLine
$utf8WithoutBom = New-Object System.Text.UTF8Encoding($false)
[IO.File]::WriteAllText($targetIntro, $serializedIntro, $utf8WithoutBom)

Write-Host "Temple Cantor installed."
Write-Host "Backup: $backupRoot"
