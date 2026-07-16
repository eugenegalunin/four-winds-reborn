param(
    [ValidateRange(1, 8)]
    [int]$SeedCount = 1,
    [string]$OutputDirectory = "diagnostics\balance-lab",
    [switch]$KeepEngineLog,
    [switch]$KeepMatchRecords
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

$runId = "{0:yyyyMMdd-HHmmss}-{1}" -f [DateTime]::UtcNow, $PID
$recordsDirectory = Join-Path $outputPath ".balance-records-$runId"
New-Item -ItemType Directory -Path $recordsDirectory | Out-Null
$engineLog = Join-Path $outputPath "balance-engine-$runId.log"
$utf8 = New-Object System.Text.UTF8Encoding($false)

function Quote-NativeArgument([string]$Value) {
    return '"' + $Value.Replace('"', '\"') + '"'
}

function Invoke-BalanceProcess([string]$Arguments) {
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $runner
    $startInfo.Arguments = $Arguments
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
    return [PSCustomObject]@{
        ExitCode = $process.ExitCode
        StandardOutput = $standardOutputTask.Result
        StandardError = $standardErrorTask.Result
    }
}

function Save-EngineDiagnostics([string]$Label, $Result, [bool]$Append) {
    $content = "===== $Label =====`r`n" + $Result.StandardOutput + $Result.StandardError
    if ($Append) {
        [IO.File]::AppendAllText($engineLog, $content, $utf8)
    } else {
        [IO.File]::WriteAllText($engineLog, $content, $utf8)
    }
}

$batchSucceeded = $false
$diagnosticsWritten = $false
try {
    $matchCount = $SeedCount * 4
    for ($index = 0; $index -lt $matchCount; ++$index) {
        $recordPath = Join-Path $recordsDirectory ("match-{0:D6}.json" -f $index)
        $arguments = "--balance-match {0} {1} {2}" -f `
            (Quote-NativeArgument $recordPath), $SeedCount, $index
        $result = Invoke-BalanceProcess $arguments

        if ($KeepEngineLog -or $result.ExitCode -ne 0) {
            Save-EngineDiagnostics ("isolated match {0}/{1}" -f ($index + 1), $matchCount) `
                $result $diagnosticsWritten
            $diagnosticsWritten = $true
        }
        if ($result.ExitCode -ne 0) {
            throw "Isolated match $($index + 1)/$matchCount failed with process exit code $($result.ExitCode). Diagnostics: $engineLog"
        }
        Write-Host ("Balance match {0}/{1}: isolated process complete" -f ($index + 1), $matchCount)
    }

    $mergeArguments = "--balance-merge {0} {1} {2}" -f `
        (Quote-NativeArgument $outputPath), (Quote-NativeArgument $recordsDirectory), $SeedCount
    $merge = Invoke-BalanceProcess $mergeArguments
    if ($KeepEngineLog -or $merge.ExitCode -ne 0) {
        Save-EngineDiagnostics "report merge" $merge $diagnosticsWritten
        $diagnosticsWritten = $true
    }
    if ($merge.ExitCode -ne 0) {
        throw "Balance report merge failed with exit code $($merge.ExitCode). Diagnostics: $engineLog"
    }

    $batchSucceeded = $true
    $humanReport = Join-Path $outputPath "balance-report.txt"
    if (Test-Path -LiteralPath $humanReport) {
        Write-Host (Get-Content -LiteralPath $humanReport -Raw)
    }
    Write-Host "Balance reports: $outputPath"
    $replayDirectory = Join-Path $outputPath "replays"
    $currentReport = Get-Content -LiteralPath (Join-Path $outputPath "balance-report.json") `
        -Raw | ConvertFrom-Json
    $retainedReplayCount = @($currentReport.matches | Where-Object {
        $_.replay_retention_reasons.Count -gt 0
    }).Count
    if ($retainedReplayCount -gt 0 -and (Test-Path -LiteralPath $replayDirectory)) {
        Write-Host "Retained failure/outlier replays: $replayDirectory"
    }
    if ($KeepEngineLog) {
        Write-Host "Engine diagnostics: $engineLog"
    }
} finally {
    if ($batchSucceeded -and -not $KeepMatchRecords) {
        $resolvedRecords = [IO.Path]::GetFullPath($recordsDirectory)
        $outputPrefix = $outputPath.TrimEnd([IO.Path]::DirectorySeparatorChar) + `
            [IO.Path]::DirectorySeparatorChar
        $ownedDirectory = $resolvedRecords.StartsWith(
            $outputPrefix, [StringComparison]::OrdinalIgnoreCase) -and
            ([IO.Path]::GetFileName($resolvedRecords) -like ".balance-records-*")
        if (-not $ownedDirectory) {
            throw "Refusing to remove unverified match-record directory '$resolvedRecords'."
        }
        Remove-Item -LiteralPath $resolvedRecords -Recurse -Force
    } elseif ($KeepMatchRecords -or -not $batchSucceeded) {
        Write-Host "Isolated match records: $recordsDirectory"
    }
}
