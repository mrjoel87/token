# Builds "SMSW TOTEM.vst3" with Visual Studio 2022 (Build Tools are enough) and CMake.
# Usage:
#   .\build.ps1                               # downloads JUCE automatically
#   .\build.ps1 -JucePath "I:\SMSW\tools\JUCE" # use a local JUCE checkout
param(
    [string]$JucePath = "",
    [string]$Config = "Release"
)
$ErrorActionPreference = "Stop"

$root  = $PSScriptRoot
$build = Join-Path $root "build"

$configureArgs = @("-S", $root, "-B", $build, "-G", "Visual Studio 17 2022", "-A", "x64")
if ($JucePath -ne "") { $configureArgs += "-DTOTEM_JUCE_PATH=$JucePath" }

cmake @configureArgs
if ($LASTEXITCODE -ne 0) { throw "CMake configure failed" }

cmake --build $build --config $Config --parallel
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

$vst3 = Join-Path $build "TOKEN_artefacts\$Config\VST3\SMSW TOTEM.vst3"
if (Test-Path $vst3) {
    Write-Host ""
    Write-Host "Built: $vst3" -ForegroundColor Cyan
    Write-Host "Install with (as Administrator): .\install.ps1" -ForegroundColor Cyan
} else {
    throw "Build finished but $vst3 was not found"
}
