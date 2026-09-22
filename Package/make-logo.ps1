# Generates Media Tags icons from a single vector drawing.
# Run from the repo: powershell -NoProfile -File Package\make-logo.ps1

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$PackageDir = $PSScriptRoot
$RepoRoot = Split-Path $PackageDir -Parent
$AssetsDir = Join-Path $PackageDir "Assets"
$WebDir = Join-Path $RepoRoot "Project1\Web"

New-Item -ItemType Directory -Force -Path $AssetsDir | Out-Null
New-Item -ItemType Directory -Force -Path $WebDir | Out-Null

function Add-RoundedRect(
    [System.Drawing.Drawing2D.GraphicsPath]$path,
    [float]$x,
    [float]$y,
    [float]$w,
    [float]$h,
    [float]$r)
{
    if ($r -lt 0.5) { $r = 0.5 }
    if ($r * 2 -gt $w) { $r = $w / 2 }
    if ($r * 2 -gt $h) { $r = $h / 2 }
    $d = $r * 2
    $path.AddArc($x, $y, $d, $d, 180, 90)
    $path.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
    $path.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
    $path.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
    $path.CloseFigure()
}

function New-LogoBitmap([int]$size)
{
    $bmp = New-Object System.Drawing.Bitmap $size, $size, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bmp.SetResolution(96, 96)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $g.Clear([System.Drawing.Color]::Transparent)

    $s = [float]$size
    $pad = [Math]::Max(1.0, $s * 0.06)
    $tileR = [Math]::Max(2.0, $s * 0.22)
    $blue = [System.Drawing.Color]::FromArgb(255, 0, 103, 192)
    $white = [System.Drawing.Color]::FromArgb(255, 255, 255, 255)

    $tile = New-Object System.Drawing.Drawing2D.GraphicsPath
    Add-RoundedRect $tile $pad $pad ($s - 2 * $pad) ($s - 2 * $pad) $tileR
    $blueBrush = New-Object System.Drawing.SolidBrush $blue
    $g.FillPath($blueBrush, $tile)

    $tag = New-Object System.Drawing.Drawing2D.GraphicsPath
    $left = $s * 0.22
    $top = $s * 0.30
    $bodyW = $s * 0.40
    $bodyH = $s * 0.40
    $bottom = $top + $bodyH
    $bodyRight = $left + $bodyW
    $tipX = $s * 0.80
    $midY = $s * 0.50
    $tagR = [Math]::Max(1.0, $s * 0.055)
    $d = $tagR * 2

    $tag.AddLine(($left + $tagR), $top, $bodyRight, $top)
    $tag.AddLine($bodyRight, $top, $tipX, $midY)
    $tag.AddLine($tipX, $midY, $bodyRight, $bottom)
    $tag.AddLine($bodyRight, $bottom, ($left + $tagR), $bottom)
    $tag.AddArc($left, ($bottom - $d), $d, $d, 90, 90)
    $tag.AddLine($left, ($bottom - $tagR), $left, ($top + $tagR))
    $tag.AddArc($left, $top, $d, $d, 180, 90)
    $tag.CloseFigure()

    $whiteBrush = New-Object System.Drawing.SolidBrush $white
    $g.FillPath($whiteBrush, $tag)

    $holeR = [Math]::Max(1.2, $s * 0.055)
    $holeX = $s * 0.32 - $holeR
    $holeY = $midY - $holeR
    $g.FillEllipse($blueBrush, $holeX, $holeY, $holeR * 2, $holeR * 2)

    $blueBrush.Dispose()
    $whiteBrush.Dispose()
    $tile.Dispose()
    $tag.Dispose()
    $g.Dispose()
    return $bmp
}

function Save-Png([System.Drawing.Bitmap]$bmp, [string]$path)
{
    $bmp.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
}

function Save-Ico([System.Drawing.Bitmap[]]$images, [string]$path)
{
    $payloads = New-Object System.Collections.Generic.List[byte[]]
    foreach ($img in $images)
    {
        $ms = New-Object System.IO.MemoryStream
        $img.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        $payloads.Add($ms.ToArray())
        $ms.Dispose()
    }

    $count = $payloads.Count
    $offset = 6 + (16 * $count)
    $fs = [System.IO.File]::Open($path, [System.IO.FileMode]::Create)
    $bw = New-Object System.IO.BinaryWriter $fs
    $bw.Write([uint16]0)
    $bw.Write([uint16]1)
    $bw.Write([uint16]$count)

    for ($i = 0; $i -lt $count; $i++)
    {
        $img = $images[$i]
        $w = if ($img.Width -ge 256) { 0 } else { $img.Width }
        $h = if ($img.Height -ge 256) { 0 } else { $img.Height }
        $bw.Write([byte]$w)
        $bw.Write([byte]$h)
        $bw.Write([byte]0)
        $bw.Write([byte]0)
        $bw.Write([uint16]1)
        $bw.Write([uint16]32)
        $bw.Write([uint32]$payloads[$i].Length)
        $bw.Write([uint32]$offset)
        $offset += $payloads[$i].Length
    }

    foreach ($png in $payloads)
    {
        $bw.Write($png)
    }

    $bw.Flush()
    $bw.Dispose()
    $fs.Dispose()
}

$sizes = @(16, 20, 24, 32, 40, 44, 48, 64, 128, 150, 256)
$bitmaps = @{}
foreach ($size in $sizes)
{
    $bitmaps[$size] = New-LogoBitmap $size
}

Save-Png $bitmaps[256] (Join-Path $AssetsDir "StoreLogo.png")
Save-Png $bitmaps[44] (Join-Path $AssetsDir "Square44x44Logo.png")
Save-Png $bitmaps[150] (Join-Path $AssetsDir "Square150x150Logo.png")
Save-Png $bitmaps[128] (Join-Path $WebDir "logo.png")

$icoImages = @(
    $bitmaps[16],
    $bitmaps[20],
    $bitmaps[24],
    $bitmaps[32],
    $bitmaps[40],
    $bitmaps[48],
    $bitmaps[64],
    $bitmaps[256]
)
Save-Ico $icoImages (Join-Path $RepoRoot "Project1\Project1.ico")
Copy-Item (Join-Path $RepoRoot "Project1\Project1.ico") (Join-Path $RepoRoot "Project1\small.ico") -Force

foreach ($bmp in $bitmaps.Values)
{
    $bmp.Dispose()
}

Write-Host "Iconos listos en Package\Assets, Project1\Web\logo.png y Project1\Project1.ico"
