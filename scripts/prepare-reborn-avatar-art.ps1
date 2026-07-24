param(
    [Parameter(Mandatory = $true)]
    [string]$Source,

    [Parameter(Mandatory = $true)]
    [string]$FullOutput,

    [Parameter(Mandatory = $true)]
    [string]$PortraitOutput,

    [Parameter(Mandatory = $true)]
    [string]$FrameTemplate,

    [string]$PortraitSource,

    [int]$PortraitX = 234,
    [int]$PortraitY = 85,
    [int]$PortraitSize = 430
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

function New-HighQualityGraphics([System.Drawing.Image]$target) {
    $graphics = [System.Drawing.Graphics]::FromImage($target)
    $graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
    $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    return $graphics
}

$sourceImage = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $Source))
$frameImage = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $FrameTemplate))
$portraitSourceImage = $null

if ($PortraitSource) {
    $portraitSourceImage = [System.Drawing.Bitmap]::FromFile((Resolve-Path -LiteralPath $PortraitSource))
}

try {
    foreach ($path in @($FullOutput, $PortraitOutput)) {
        $directory = Split-Path -Parent $path
        if ($directory) {
            New-Item -ItemType Directory -Force -Path $directory | Out-Null
        }
    }

    # The legacy card carries its right and bottom UI edge inside the bitmap.
    # Keep that two-pixel edge while replacing the 262x514 artwork interior.
    $full = New-Object System.Drawing.Bitmap 264, 516, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $fullGraphics = New-HighQualityGraphics $full
    try {
        $fullGraphics.DrawImage($frameImage, [System.Drawing.Rectangle]::new(0, 0, 264, 516))
        $fullGraphics.DrawImage(
            $sourceImage,
            [System.Drawing.Rectangle]::new(0, 0, 262, 514),
            [System.Drawing.Rectangle]::new(0, 0, $sourceImage.Width, $sourceImage.Height),
            [System.Drawing.GraphicsUnit]::Pixel
        )
    }
    finally {
        $fullGraphics.Dispose()
    }
    $full.Save($FullOutput, [System.Drawing.Imaging.ImageFormat]::Png)
    $full.Dispose()

    $portraitInput = if ($portraitSourceImage) { $portraitSourceImage } else { $sourceImage }
    $portraitCrop = if ($portraitSourceImage) {
        $side = [Math]::Min($portraitInput.Width, $portraitInput.Height)
        [System.Drawing.Rectangle]::new(
            [int](($portraitInput.Width - $side) / 2),
            [int](($portraitInput.Height - $side) / 2),
            $side,
            $side
        )
    }
    else {
        if ($PortraitX -lt 0 -or $PortraitY -lt 0 -or
            $PortraitX + $PortraitSize -gt $sourceImage.Width -or
            $PortraitY + $PortraitSize -gt $sourceImage.Height) {
            throw "Portrait crop lies outside the source image."
        }

        [System.Drawing.Rectangle]::new($PortraitX, $PortraitY, $PortraitSize, $PortraitSize)
    }

    $portrait = New-Object System.Drawing.Bitmap 140, 140, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $portraitGraphics = New-HighQualityGraphics $portrait
    try {
        $portraitGraphics.DrawImage(
            $portraitInput,
            [System.Drawing.Rectangle]::new(0, 0, 140, 140),
            $portraitCrop,
            [System.Drawing.GraphicsUnit]::Pixel
        )
    }
    finally {
        $portraitGraphics.Dispose()
    }
    $portrait.Save($PortraitOutput, [System.Drawing.Imaging.ImageFormat]::Png)
    $portrait.Dispose()
}
finally {
    if ($portraitSourceImage) {
        $portraitSourceImage.Dispose()
    }
    $frameImage.Dispose()
    $sourceImage.Dispose()
}

Write-Host "Prepared Reborn avatar art:"
Write-Host "  full:     $FullOutput"
Write-Host "  portrait: $PortraitOutput"
