$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$OutFile = Join-Path $root "assets\ICON0.PNG"
$width = 144
$height = 80

function Get-Crc32([byte[]]$data) {
    $crc = [uint32]0xFFFFFFFF
    foreach ($b in $data) {
        $crc = $crc -bxor $b
        for ($i = 0; $i -lt 8; $i++) {
            if ($crc -band 1) { $crc = [uint32](($crc -shr 1) -bxor 0xEDB88320) }
            else { $crc = [uint32]($crc -shr 1) }
        }
    }
    return [uint32]($crc -bxor 0xFFFFFFFF)
}

function Get-Adler32([byte[]]$data) {
    $a = [uint32]1
    $b = [uint32]0
    foreach ($x in $data) {
        $a = ($a + [uint32]$x) % [uint32]65521
        $b = ($b + $a) % [uint32]65521
    }
    return [uint32](($b * [uint32]65536) + $a)
}

function New-Chunk([string]$type, [byte[]]$data) {
    $tag = [Text.Encoding]::ASCII.GetBytes($type)
    $len = [BitConverter]::GetBytes([uint32]$data.Length)
    if ([BitConverter]::IsLittleEndian) { [Array]::Reverse($len) }
    $crcSrc = New-Object byte[] ($tag.Length + $data.Length)
    [Array]::Copy($tag, 0, $crcSrc, 0, $tag.Length)
    if ($data.Length -gt 0) { [Array]::Copy($data, 0, $crcSrc, $tag.Length, $data.Length) }
    $crc = [BitConverter]::GetBytes((Get-Crc32 $crcSrc))
    if ([BitConverter]::IsLittleEndian) { [Array]::Reverse($crc) }
    return $len + $tag + $data + $crc
}

$raw = New-Object System.Collections.Generic.List[byte]
for ($y = 0; $y -lt $height; $y++) {
    [void]$raw.Add(0)
    for ($x = 0; $x -lt $width; $x++) {
        $t = $x / [double]($width - 1)
        $r = [byte](20 + 30 * $t)
        $g = [byte](90 + 80 * $t)
        $b = [byte](140 + 70 * (1 - $t))
        if ($y -gt 34 -and $y -lt 46 -and $x -gt 18 -and $x -lt 126) {
            $r = 240; $g = 244; $b = 232
        }
        [void]$raw.Add($r)
        [void]$raw.Add($g)
        [void]$raw.Add($b)
    }
}
$rawBytes = [byte[]]$raw.ToArray()

$ms = New-Object IO.MemoryStream
[void]$ms.WriteByte(0x78)
[void]$ms.WriteByte(0x01)
$offset = 0
while ($offset -lt $rawBytes.Length) {
    $n = [Math]::Min(65535, $rawBytes.Length - $offset)
    $final = if (($offset + $n) -ge $rawBytes.Length) { 1 } else { 0 }
    [void]$ms.WriteByte([byte]$final)
    [void]$ms.WriteByte([byte]($n -band 0xFF))
    [void]$ms.WriteByte([byte](($n -shr 8) -band 0xFF))
    $ninv = 65535 - $n
    [void]$ms.WriteByte([byte]($ninv -band 0xFF))
    [void]$ms.WriteByte([byte](($ninv -shr 8) -band 0xFF))
    $ms.Write($rawBytes, $offset, $n)
    $offset += $n
}
$adler = [BitConverter]::GetBytes((Get-Adler32 $rawBytes))
if ([BitConverter]::IsLittleEndian) { [Array]::Reverse($adler) }
$ms.Write($adler, 0, 4)
$idat = $ms.ToArray()

$ihdr = New-Object byte[] 13
$w = [BitConverter]::GetBytes([uint32]$width)
$h = [BitConverter]::GetBytes([uint32]$height)
if ([BitConverter]::IsLittleEndian) { [Array]::Reverse($w); [Array]::Reverse($h) }
[Array]::Copy($w, 0, $ihdr, 0, 4)
[Array]::Copy($h, 0, $ihdr, 4, 4)
$ihdr[8] = 8
$ihdr[9] = 2

$png = [byte[]](0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A)
$png += New-Chunk "IHDR" $ihdr
$png += New-Chunk "IDAT" $idat
$png += New-Chunk "IEND" ([byte[]]@())

New-Item -ItemType Directory -Force -Path (Split-Path $OutFile) | Out-Null
[IO.File]::WriteAllBytes($OutFile, $png)
Write-Host "wrote $OutFile ($($png.Length) bytes)"
