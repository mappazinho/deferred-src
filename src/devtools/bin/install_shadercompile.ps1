[CmdletBinding()]
param (
    [switch]$Force
)

$ErrorActionPreference = "Stop"

# Pinned to the release built from the same upstream commit as the integration
# scripts/header in this repository. Update this tag deliberately when updating
# the vendored SCell555 integration files.
$releaseTag = "build_235_20231013.2"
$downloadUrl = "https://github.com/SCell555/ShaderCompile/releases/download/$releaseTag/ShaderCompile.exe"
$destination = Join-Path $PSScriptRoot "ShaderCompile.exe"
$tempFile = "$destination.download"

if ((Test-Path $destination) -and -not $Force) {
    Write-Host "SCell555 ShaderCompile is already installed: $destination"
    exit 0
}

Write-Host "Installing SCell555 ShaderCompile $releaseTag..."
try {
    Invoke-WebRequest -UseBasicParsing -Uri $downloadUrl -OutFile $tempFile

    if (-not (Test-Path $tempFile) -or (Get-Item $tempFile).Length -eq 0) {
        throw "Downloaded ShaderCompile.exe is empty or missing."
    }

    Move-Item -Force $tempFile $destination
    Write-Host "Installed ShaderCompile.exe to $destination"
}
catch {
    if (Test-Path $tempFile) {
        Remove-Item -Force $tempFile
    }
    Write-Error $_
    exit 1
}
