param([string]$Output = "$PSScriptRoot/../_data/doom1.wad")
$ErrorActionPreference = "Stop"
# Only the Doom 1.9 shareware episode, never the retail WADs.
$url = "https://raw.githubusercontent.com/Akbar30Bill/DOOM_wads/master/doom1.wad"
$expected = "1d7d43be501e67d927e415e0b8f3e29c3bf33075e859721816f652a526cac771"
$Output = [IO.Path]::GetFullPath($Output)
New-Item -ItemType Directory -Force (Split-Path $Output) | Out-Null
Invoke-WebRequest -Uri $url -OutFile "$Output.download"
if ((Get-FileHash "$Output.download" -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    throw "Shareware checksum mismatch; download retained separately for inspection"
}
Move-Item -LiteralPath "$Output.download" -Destination $Output -Force
Write-Host "Verified Doom 1.9 shareware: $Output"
