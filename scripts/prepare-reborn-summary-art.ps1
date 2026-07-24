param(
    [string]$ClassicDir = "themes/classic/assets/images",
    [string]$OutputDir = "themes/reborn/assets/images"
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

function Get-Noise([int]$x, [int]$y) {
    $value = (($x * 73856093) -bxor ($y * 19349663) -bxor (($x + $y) * 83492791))
    return (($value -band 255) / 255.0)
}

function Convert-SummaryImage([string]$Name) {
    $sourcePath = Join-Path $ClassicDir $Name
    $targetPath = Join-Path $OutputDir $Name
    $resolvedSource = (Resolve-Path $sourcePath).Path
    $source = [System.Drawing.Bitmap]::new($resolvedSource)
    $output = [System.Drawing.Bitmap]::new($source.Width, $source.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)

    try {
        for ($y = 0; $y -lt $source.Height; $y++) {
            for ($x = 0; $x -lt $source.Width; $x++) {
                $c = $source.GetPixel($x, $y)
                $isColorKey = ($c.R -ge 248 -and $c.G -ge 198 -and $c.B -ge 198)
                $isGreenTexture = ($c.G -gt ($c.R + 5) -and $c.G -gt ($c.B + 4) -and $c.R -lt 105 -and $c.B -lt 105)
                $isGoldBorder = ($c.R -gt 72 -and $c.G -gt 64 -and $c.B -lt 105 -and $c.R -gt ($c.B * 1.35))

                if ($isColorKey) {
                    $output.SetPixel($x, $y, $c)
                    continue
                }

                if ($isGreenTexture) {
                    $noise = Get-Noise $x $y
                    $grain = [int](($noise - 0.5) * 10)
                    $wave = [int](3.0 * [Math]::Sin(($x + $y) / 37.0) + 2.0 * [Math]::Sin(($x - $y) / 71.0))
                    $edgeX = [Math]::Abs(($x / [double]($source.Width - 1)) - 0.5) * 2.0
                    $edgeY = [Math]::Abs(($y / [double]($source.Height - 1)) - 0.5) * 2.0
                    $vignette = [int](10.0 * [Math]::Min(1.0, (($edgeX * $edgeX) + ($edgeY * $edgeY)) / 1.45))
                    $base = 28 + $grain + $wave - $vignette
                    $r = [Math]::Max(9, [Math]::Min(255, $base + 2))
                    $g = [Math]::Max(12, [Math]::Min(255, $base + 8))
                    $b = [Math]::Max(14, [Math]::Min(255, $base + 11))
                    $output.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($c.A, $r, $g, $b))
                    continue
                }

                if ($isGoldBorder) {
                    $luma = [Math]::Max(0.0, [Math]::Min(1.0, (($c.R + $c.G) / 2.0) / 255.0))
                    $r = [int](54 + 174 * $luma)
                    $g = [int](43 + 126 * $luma)
                    $b = [int](31 + 54 * $luma)
                    $output.SetPixel($x, $y, [System.Drawing.Color]::FromArgb($c.A, $r, $g, $b))
                    continue
                }

                $output.SetPixel($x, $y, $c)
            }
        }

        $output.Save($targetPath, [System.Drawing.Imaging.ImageFormat]::Png)
        Write-Host "Prepared $targetPath"
    }
    finally {
        $output.Dispose()
        $source.Dispose()
    }
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
@("mapsummary.png", "scoreall.png", "scorechar.png") | ForEach-Object { Convert-SummaryImage $_ }
