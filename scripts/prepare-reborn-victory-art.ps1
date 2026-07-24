param(
    [Parameter(Mandatory = $true)]
    [string] $Source,

    [Parameter(Mandatory = $true)]
    [ValidateRange(0, 8)]
    [int] $Index
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$repoRoot = Split-Path -Parent $PSScriptRoot
$output = Join-Path $repoRoot "themes/reborn/assets/images/gloat$Index.png"
$sourcePath = (Resolve-Path -LiteralPath $Source).Path

$image = [System.Drawing.Image]::FromFile($sourcePath)
try {
    $targetRatio = 4.0 / 3.0
    $sourceRatio = $image.Width / [double] $image.Height

    if ($sourceRatio -gt $targetRatio) {
        $cropHeight = $image.Height
        $cropWidth = [int] [Math]::Round($cropHeight * $targetRatio)
        $cropX = [int] (($image.Width - $cropWidth) / 2)
        $cropY = 0
    }
    else {
        $cropWidth = $image.Width
        $cropHeight = [int] [Math]::Round($cropWidth / $targetRatio)
        $cropX = 0
        $cropY = [int] (($image.Height - $cropHeight) / 2)
    }

    $bitmap = New-Object System.Drawing.Bitmap 1024, 768
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
            $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
            $graphics.DrawImage(
                $image,
                (New-Object System.Drawing.Rectangle 0, 0, 1024, 768),
                (New-Object System.Drawing.Rectangle $cropX, $cropY, $cropWidth, $cropHeight),
                [System.Drawing.GraphicsUnit]::Pixel
            )
        }
        finally {
            $graphics.Dispose()
        }

        $bitmap.Save($output, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $bitmap.Dispose()
    }
}
finally {
    $image.Dispose()
}

Write-Host "Prepared gloat$Index.png from $sourcePath"
