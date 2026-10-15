[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$InstallDirectory,
    [string]$PackageDirectory = $PSScriptRoot,
    # Stage the launcher so it can obtain the supported build with Steam login.
    # This never bypasses the launcher's game executable verification.
    [switch]$PrepareGame
)
$ErrorActionPreference = 'Stop'
$package = (Resolve-Path -LiteralPath $PackageDirectory).Path
$install = [IO.Path]::GetFullPath($InstallDirectory)
$manifest = Get-Content -LiteralPath (Join-Path $package 'vr-package.json') -Raw | ConvertFrom-Json
if ($manifest.schema -ne 1) { throw 'Unsupported package manifest' }
foreach ($file in $manifest.files) {
    $path = [IO.Path]::GetFullPath((Join-Path $package $file.path))
    if (-not $path.StartsWith($package.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Package path escapes its directory'
    }
    if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.sha256) {
        throw "Package checksum mismatch: $($file.path)"
    }
}
$active = Get-Process Skate,ReSkateLauncher -ErrorAction SilentlyContinue | Where-Object {
    $_.Path -and [IO.Path]::GetDirectoryName($_.Path).TrimEnd('\').Equals(
        $install.TrimEnd('\'), [StringComparison]::OrdinalIgnoreCase)
}
if ($active) { throw 'Close skate. and ReSkateLauncher in the target installation before installing' }
$game = Join-Path $install 'Skate.exe'
$supported = (Test-Path -LiteralPath $game) -and
    ((Get-FileHash -LiteralPath $game -Algorithm SHA256).Hash.ToLowerInvariant() -eq $manifest.gameSha256)
if (-not $supported -and -not $PrepareGame) {
    throw 'The game does not match build 25414733. Use -PrepareGame and run the launcher to install the supported game first.'
}
New-Item -ItemType Directory -Path $install -Force | Out-Null
$backup = Join-Path $install ('VR-backups/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
foreach ($name in @('ReSkateLauncher.exe','ReSkate.dll','ReSkateVRSetup.exe','openxr_loader.dll','vr-package.json','VR_V1.md','LICENSE')) {
    $destination = Join-Path $install $name
    if (Test-Path -LiteralPath $destination) {
        New-Item -ItemType Directory -Path $backup -Force | Out-Null
        Copy-Item -LiteralPath $destination -Destination $backup
    }
    Copy-Item -LiteralPath (Join-Path $package $name) -Destination $destination
}
Copy-Item -LiteralPath (Join-Path $package 'licenses') -Destination $install -Recurse -Force
foreach ($name in @('ReSkateLauncher.exe','ReSkate.dll','ReSkateVRSetup.exe','openxr_loader.dll')) {
    if ((Get-FileHash -LiteralPath (Join-Path $install $name)).Hash -ne
        (Get-FileHash -LiteralPath (Join-Path $package $name)).Hash) { throw "Install verification failed: $name" }
}
if ($supported) { "VR development candidate installed and verified at $install" }
else { "VR files staged at $install. Run ReSkateLauncher.exe to obtain supported game build 25414733; game compatibility remains unverified." }
