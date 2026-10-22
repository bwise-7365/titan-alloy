# Copyright Ben Paul Wise. All Rights Reserved.
#
# Combine one or more .ico files (each typically a single image) into one
# multi-resolution .ico, so Windows can pick the best size per context (16 for the
# taskbar/list view, 32/48 for medium/large, 256 for the jumbo view). No external
# tools: a .ico is a 6-byte header + one 16-byte directory entry per image + the
# concatenated image payloads, so merging is just re-indexing the entries.
#
#   powershell -File merge-ico.ps1 -Out out.ico -In a16.ico,a32.ico,a48.ico,a256.ico
#
param(
    [Parameter(Mandatory = $true)][string] $Out,
    [Parameter(Mandatory = $true)][string] $In   # comma-separated list of input .ico paths
)
$ErrorActionPreference = 'Stop'

# One comma-joined argument (not -File's space-separated array, which mis-binds).
$inputFiles = $In -split ','

$heads = New-Object System.Collections.Generic.List[byte[]]  # 8-byte dims/planes/bits per image
$blobs = New-Object System.Collections.Generic.List[byte[]]  # the image payload bytes

foreach ($path in $inputFiles) {
    $b = [System.IO.File]::ReadAllBytes($path)
    if ($b.Length -lt 6 -or
        [BitConverter]::ToUInt16($b, 0) -ne 0 -or   # reserved
        [BitConverter]::ToUInt16($b, 2) -ne 1) {     # type 1 == icon
        throw "Not a valid .ico: $path"
    }
    $count = [BitConverter]::ToUInt16($b, 4)
    for ($i = 0; $i -lt $count; $i++) {
        $e   = 6 + $i * 16
        $len = [int][BitConverter]::ToUInt32($b, $e + 8)
        $off = [int][BitConverter]::ToUInt32($b, $e + 12)
        $head = New-Object byte[] 8                  # width,height,colors,reserved,planes,bits
        [Array]::Copy($b, [int]$e, $head, 0, 8)
        $img = New-Object byte[] $len
        [Array]::Copy($b, $off, $img, 0, $len)
        $heads.Add($head)
        $blobs.Add($img)
    }
}

$n  = $heads.Count
$fs = [System.IO.File]::Open($Out, [System.IO.FileMode]::Create)
$bw = New-Object System.IO.BinaryWriter($fs)
try {
    $bw.Write([uint16]0)    # reserved
    $bw.Write([uint16]1)    # type: icon
    $bw.Write([uint16]$n)   # image count
    $offset = 6 + 16 * $n   # payloads start after the directory
    for ($i = 0; $i -lt $n; $i++) {
        $bw.Write($heads[$i])                   # 8 bytes copied verbatim
        $bw.Write([uint32]$blobs[$i].Length)    # bytesInRes
        $bw.Write([uint32]$offset)              # imageOffset
        $offset += $blobs[$i].Length
    }
    foreach ($blob in $blobs) { $bw.Write($blob) }
}
finally {
    $bw.Close()
    $fs.Close()
}
Write-Host "Wrote $Out ($n image(s))."
# Copyright Ben Paul Wise. All Rights Reserved.
