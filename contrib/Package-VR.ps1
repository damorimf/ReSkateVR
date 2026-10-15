[CmdletBinding()]
param(
    [string]$BuildDirectory = 'build/vr-v1/Release',
    [string]$OutputDirectory = 'artifacts/vr-v1'
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$build = [IO.Path]::GetFullPath((Join-Path $repo $BuildDirectory))
$output = [IO.Path]::GetFullPath((Join-Path $repo $OutputDirectory))
foreach ($name in @('ReSkateLauncher.exe', 'ReSkate.dll', 'ReSkateVRSetup.exe')) {
    if (-not (Test-Path -LiteralPath (Join-Path $build $name))) { throw "Missing build output: $name" }
}
$commit = (git -C $repo rev-parse HEAD).Trim()
if ($LASTEXITCODE) { throw 'Cannot identify source revision' }
$expectedRevision = (git -C $repo describe --always --dirty --abbrev=12).Trim()
$configuration = Join-Path (Split-Path $build -Parent) 'generated/crash_report_config.h'
$configured = [regex]::Match((Get-Content -LiteralPath $configuration -Raw), 'build_version\[\] = "([^"]+)"').Groups[1].Value
if ($configured -ne $expectedRevision) {
    throw 'Build revision differs from the source tree. Reconfigure and rebuild before packaging.'
}
$stage = Join-Path $output ('ReSkateVR-1.0.0-dev-' + $commit.Substring(0, 8))
if (Test-Path -LiteralPath $stage) { throw "Package directory already exists: $stage" }
New-Item -ItemType Directory -Path $stage -Force | Out-Null
$download = Join-Path $output 'openxr_loader_windows-1.1.63.zip'
$expected = '01c631aeabbfe0879540f77ef833416c532a20746285b494630160c23588b771'
if (-not (Test-Path -LiteralPath $download)) {
    Invoke-WebRequest 'https://github.com/KhronosGroup/OpenXR-SDK-Source/releases/download/release-1.1.63/openxr_loader_windows-1.1.63.zip' -OutFile $download
}
if ((Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expected) {
    throw 'Pinned OpenXR loader archive checksum mismatch'
}
$loader = Join-Path $output 'openxr-1.1.63'
Expand-Archive -LiteralPath $download -DestinationPath $loader -Force
Copy-Item -LiteralPath (Join-Path $loader 'x64/bin/openxr_loader.dll') -Destination $stage
foreach ($name in @('ReSkateLauncher.exe', 'ReSkate.dll', 'ReSkateVRSetup.exe')) {
    Copy-Item -LiteralPath (Join-Path $build $name) -Destination $stage
}
Copy-Item -LiteralPath (Join-Path $repo 'LICENSE'), (Join-Path $repo 'VR_V1.md') -Destination $stage
Copy-Item -LiteralPath (Join-Path $repo 'contrib/Install-VR.ps1') -Destination $stage
$licenses = Join-Path $stage 'licenses'
New-Item -ItemType Directory -Path $licenses | Out-Null
Copy-Item -LiteralPath (Join-Path $repo 'External/README.md'), (Join-Path $repo 'External/manifest.json') -Destination $licenses
$external = Join-Path $repo 'External'
foreach ($file in Get-ChildItem -LiteralPath $external -Recurse -File | Where-Object Name -Match '^(LICENSE|LICENCE|COPYING|NOTICE|OFL)([._-]|$)') {
    $relative = $file.FullName.Substring($external.Length + 1)
    $destination = Join-Path $licenses $relative
    New-Item -ItemType Directory -Path (Split-Path $destination -Parent) -Force | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $destination
}
$files = @(Get-ChildItem -LiteralPath $stage -Recurse -File | ForEach-Object {
    [ordered]@{ path = $_.FullName.Substring($stage.Length + 1).Replace('\', '/'); sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
})
$manifest = [ordered]@{
    schema = 1; version = '1.0.0-dev'; source = $commit; buildRevision = $configured
    sourceDirty = [bool](git -C $repo status --porcelain)
    verifiedV1 = $false; supportedGameBuild = '25414733'
    gameSha256 = 'fbce74d5e28ef525dbba2cb4adbebc13405bdbd88f31bc940bca45e4ae88b8f9'
    openxrVersion = '1.1.63'; files = $files
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $stage 'vr-package.json') -Encoding UTF8
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath ($stage + '.zip')
Write-Output $stage
Write-Output ($stage + '.zip')
