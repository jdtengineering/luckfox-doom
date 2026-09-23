param(
    [string]$Target = "root@luckfox",
    [string]$Binary = "$PSScriptRoot/../_unix/game/doom-ascii",
    [Parameter(Mandatory=$true)][string]$Wad
)
$ErrorActionPreference = "Stop"
foreach ($file in @($Binary, $Wad)) {
    if (!(Test-Path -LiteralPath $file -PathType Leaf)) { throw "Missing file: $file" }
}
& ssh $Target 'mkdir -p /opt/doom'
if ($LASTEXITCODE) { throw "Cannot create install directory" }
& scp -O $Binary "${Target}:/opt/doom/doom-ascii.new"
if ($LASTEXITCODE) { throw "Binary upload failed" }
& scp -O $Wad "${Target}:/opt/doom/doom1.wad"
if ($LASTEXITCODE) { throw "WAD upload failed" }
& scp -O "$PSScriptRoot/../src/.default.cfg" "${Target}:/opt/doom/.default.cfg.dist"
if ($LASTEXITCODE) { throw "Config upload failed" }
& scp -O "$PSScriptRoot/doom" "$PSScriptRoot/doom-pixels" "${Target}:/usr/bin/"
if ($LASTEXITCODE) { throw "Launcher upload failed" }
& ssh $Target 'set -e; cd /opt/doom; chmod 755 doom-ascii.new /usr/bin/doom /usr/bin/doom-pixels; mv doom-ascii.new doom-ascii; test -f .default.cfg || cp .default.cfg.dist .default.cfg; sha256sum doom-ascii'
if ($LASTEXITCODE) { throw "Install failed" }
Write-Host "Play: ssh -t $Target doom"
