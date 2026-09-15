[CmdletBinding()]
param (
    [Parameter(Mandatory=$true, ValueFromPipeline=$true)][System.IO.FileInfo]$ShaderList,
    [Parameter(Mandatory=$true)][string]$Version,
    [Parameter(Mandatory=$false)][switch]$Dynamic,
    [Parameter(Mandatory=$false)][System.UInt32]$Threads
)

$ErrorActionPreference = "Stop"

if ($Version -notin @("20b", "30", "40", "41", "50", "51")) {
    throw "Unsupported shader model version: $Version"
}

$compiler = Join-Path $PSScriptRoot "ShaderCompile.exe"
if (-not (Test-Path $compiler)) {
    throw "ShaderCompile.exe was not found at $compiler"
}

$fileList = $ShaderList.OpenText()
try {
    while ($null -ne ($line = $fileList.ReadLine())) {
        if ($line -match '^\s*$' -or $line -match '^\s*//') {
            continue
        }

        $arguments = @()
        if ($Dynamic) {
            $arguments += "-dynamic"
        }
        if ($Threads -ne 0) {
            $arguments += @("-threads", $Threads)
        }
        $arguments += @("-ver", $Version, "-shaderpath", $ShaderList.DirectoryName, $line)

        & $compiler @arguments
        if ($LASTEXITCODE -ne 0) {
            throw "ShaderCompile failed for '$line' with exit code $LASTEXITCODE"
        }
    }
}
finally {
    $fileList.Close()
}
