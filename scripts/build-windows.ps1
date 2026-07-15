param(
    [switch]$InstallDeps,
    [switch]$Clean,
    [switch]$NoTests,
    [switch]$DebugBuild,
    [string]$MsysRoot = ""
)

$ErrorActionPreference = "Stop"
if (-not $MsysRoot) {
    $MsysRoot = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { "C:\msys64" }
}

$bash = Join-Path $MsysRoot "usr\bin\bash.exe"
$cygpath = Join-Path $MsysRoot "usr\bin\cygpath.exe"

if (-not (Test-Path $bash) -or -not (Test-Path $cygpath)) {
    throw "MSYS2 was not found in '$MsysRoot'. Install it from https://www.msys2.org or pass -MsysRoot."
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$unixRepo = (& $cygpath -u $repoRoot).Trim()
$unixRepo = $unixRepo.Replace("'", "'\''")

$options = @()
if ($InstallDeps) { $options += "--install-deps" }
if ($Clean) { $options += "--clean" }
if ($NoTests) { $options += "--no-tests" }
if ($DebugBuild) { $options += "--debug" }

$command = "export PATH=/ucrt64/bin:/usr/bin:`$PATH; cd '$unixRepo' && ./scripts/build-windows-msys2.sh $($options -join ' ')"
$previousMsystem = $env:MSYSTEM
$previousChere = $env:CHERE_INVOKING

try {
    $env:MSYSTEM = "UCRT64"
    $env:CHERE_INVOKING = "1"
    & $bash -lc $command
}
finally {
    $env:MSYSTEM = $previousMsystem
    $env:CHERE_INVOKING = $previousChere
}

if ($LASTEXITCODE -ne 0) {
    throw "Windows build failed with exit code $LASTEXITCODE."
}

Write-Host "Built: $repoRoot\dist\windows\four-winds-reborn.exe"
