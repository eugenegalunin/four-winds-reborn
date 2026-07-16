param(
    [Parameter(Mandatory = $true)]
    [string]$Path
)

$ErrorActionPreference = "Stop"
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$runner = Join-Path $repoRoot "build-windows\gameplay_regression_tests.exe"
$runtimeDirectory = Join-Path $repoRoot "dist\windows"

if (-not (Test-Path -LiteralPath $runner -PathType Leaf)) {
    & (Join-Path $PSScriptRoot "build-windows.ps1")
    if ($LASTEXITCODE -ne 0) {
        throw "Windows build failed with exit code $LASTEXITCODE."
    }
}
if (-not (Test-Path -LiteralPath $runtimeDirectory -PathType Container)) {
    throw "Packaged Windows runtime was not found in '$runtimeDirectory'."
}

$resolvedPath = (Resolve-Path -LiteralPath $Path).Path
$replays = if (Test-Path -LiteralPath $resolvedPath -PathType Leaf) {
    @(Get-Item -LiteralPath $resolvedPath)
} else {
    @(Get-ChildItem -LiteralPath $resolvedPath -Recurse -File -Filter "match-*.json" |
        Where-Object { $_.Directory.Name -eq "replays" } |
        Sort-Object FullName)
}

if ($replays.Count -eq 0) {
    throw "No retained match replays were found under '$resolvedPath'."
}

function Quote-NativeArgument([string]$Value) {
    return '"' + $Value.Replace('"', '\"') + '"'
}

for ($index = 0; $index -lt $replays.Count; ++$index) {
    $replay = $replays[$index]
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $runner
    $startInfo.Arguments = "--verify-balance-replay {0}" -f `
        (Quote-NativeArgument $replay.FullName)
    $startInfo.WorkingDirectory = $runtimeDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    if (-not $process.Start()) {
        throw "Replay verifier did not start for '$($replay.FullName)'."
    }
    $standardOutputTask = $process.StandardOutput.ReadToEndAsync()
    $standardErrorTask = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $output = $standardOutputTask.Result.Trim()
    $errorOutput = $standardErrorTask.Result.Trim()

    if ($process.ExitCode -ne 0) {
        throw "Replay $($index + 1)/$($replays.Count) failed: $($replay.FullName)`n$output`n$errorOutput"
    }
    Write-Host ("Replay {0}/{1}: {2}" -f ($index + 1), $replays.Count, $output)
}

Write-Host "Verified $($replays.Count) retained balance replay(s)."
