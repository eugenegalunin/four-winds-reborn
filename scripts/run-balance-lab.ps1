param(
    [ValidateRange(1, 8)]
    [int]$SeedCount = 1,
    [string]$OutputDirectory = "diagnostics\balance-lab",
    [switch]$KeepEngineLog
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$runner = Join-Path $repoRoot "build-windows\gameplay_regression_tests.exe"
$runtimeDirectory = Join-Path $repoRoot "dist\windows"

if (-not (Test-Path -LiteralPath $runner)) {
    & (Join-Path $PSScriptRoot "build-windows.ps1")
    if ($LASTEXITCODE -ne 0) {
        throw "Windows build failed with exit code $LASTEXITCODE."
    }
}
if (-not (Test-Path -LiteralPath $runtimeDirectory)) {
    throw "Packaged Windows runtime was not found in '$runtimeDirectory'."
}

$outputPath = if ([IO.Path]::IsPathRooted($OutputDirectory)) {
    [IO.Path]::GetFullPath($OutputDirectory)
} else {
    [IO.Path]::GetFullPath((Join-Path $repoRoot $OutputDirectory))
}

if (-not (Test-Path -LiteralPath $outputPath)) {
    New-Item -ItemType Directory -Path $outputPath -Force | Out-Null
}

$engineLog = Join-Path $outputPath "balance-engine.log"
$quotedOutputPath = '"' + $outputPath.Replace('"', '\"') + '"'
$startInfo = New-Object System.Diagnostics.ProcessStartInfo
$startInfo.FileName = $runner
$startInfo.Arguments = "--balance-lab $quotedOutputPath $SeedCount"
$startInfo.WorkingDirectory = $runtimeDirectory
$startInfo.UseShellExecute = $false
$startInfo.CreateNoWindow = $true
$startInfo.RedirectStandardOutput = $true
$startInfo.RedirectStandardError = $true

$process = New-Object System.Diagnostics.Process
$process.StartInfo = $startInfo
if (-not $process.Start()) {
    throw "Balance laboratory process did not start."
}
$standardOutputTask = $process.StandardOutput.ReadToEndAsync()
$standardErrorTask = $process.StandardError.ReadToEndAsync()
$process.WaitForExit()
$standardOutput = $standardOutputTask.Result
$standardError = $standardErrorTask.Result

if ($KeepEngineLog -or $process.ExitCode -ne 0) {
    $utf8 = New-Object System.Text.UTF8Encoding($false)
    [IO.File]::WriteAllText($engineLog, $standardOutput + $standardError, $utf8)
}
if ($process.ExitCode -ne 0) {
    throw "Balance laboratory failed with exit code $($process.ExitCode). Diagnostics: $engineLog"
}

$humanReport = Join-Path $outputPath "balance-report.txt"
if (Test-Path -LiteralPath $humanReport) {
    Write-Host (Get-Content -LiteralPath $humanReport -Raw)
}
Write-Host "Balance reports: $outputPath"
if ($KeepEngineLog) {
    Write-Host "Engine diagnostics: $engineLog"
}
