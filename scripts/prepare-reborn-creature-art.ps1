param(
    [Parameter(Mandatory = $true)]
    [string]$Source,

    [Parameter(Mandatory = $true)]
    [string]$Output,

    [int]$Size = 140,
    [double]$CropScale = 1.0,
    [double]$CenterX = 0.5,
    [double]$CenterY = 0.5
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

if ($CropScale -le 0.0 -or $CropScale -gt 1.0) {
    throw "CropScale must be greater than 0 and at most 1."
}
if ($CenterX -lt 0.0 -or $CenterX -gt 1.0 -or $CenterY -lt 0.0 -or $CenterY -gt 1.0) {
    throw "CenterX and CenterY must be between 0 and 1."
}

$sourceImage = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $Source))
try {
    $cropSide = [int]([Math]::Min($sourceImage.Width, $sourceImage.Height) * $CropScale)
    $centerPixelX = $sourceImage.Width * $CenterX
    $centerPixelY = $sourceImage.Height * $CenterY
    $cropX = [int][Math]::Round($centerPixelX - $cropSide / 2.0)
    $cropY = [int][Math]::Round($centerPixelY - $cropSide / 2.0)
    $cropX = [Math]::Max(0, [Math]::Min($cropX, $sourceImage.Width - $cropSide))
    $cropY = [Math]::Max(0, [Math]::Min($cropY, $sourceImage.Height - $cropSide))

    $outputDirectory = Split-Path -Parent $Output
    if ($outputDirectory) {
        New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
    }

    $target = New-Object System.Drawing.Bitmap $Size, $Size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $graphics = [System.Drawing.Graphics]::FromImage($target)
        try {
            $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
            $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
            $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
            $graphics.DrawImage(
                $sourceImage,
                [System.Drawing.Rectangle]::new(0, 0, $Size, $Size),
                [System.Drawing.Rectangle]::new($cropX, $cropY, $cropSide, $cropSide),
                [System.Drawing.GraphicsUnit]::Pixel
            )
        }
        finally {
            $graphics.Dispose()
        }
        $target.Save($Output, [System.Drawing.Imaging.ImageFormat]::Png)
    }
    finally {
        $target.Dispose()
    }
}
finally {
    $sourceImage.Dispose()
}

Write-Host "Prepared Reborn creature art: $Output"
