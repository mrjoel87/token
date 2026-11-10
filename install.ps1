# Copies the built TOKEN.vst3 into the standard Windows VST3 folder.
# Run from an Administrator PowerShell (Program Files is protected).
param([string]$Config = "Release")
$ErrorActionPreference = "Stop"

$src = Join-Path $PSScriptRoot "build\TOKEN_artefacts\$Config\VST3\TOKEN.vst3"
$dst = "C:\Program Files\Common Files\VST3"

if (-not (Test-Path $src)) { throw "Not built yet: $src (run .\build.ps1 first)" }

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) { throw "Please run this script from an Administrator PowerShell." }

$target = Join-Path $dst "TOKEN.vst3"
if (Test-Path $target) { Remove-Item -Recurse -Force $target }
Copy-Item -Recurse -Force $src $dst
Write-Host "Installed to $target" -ForegroundColor Cyan
Write-Host "In Ableton Live 11: Preferences > Plug-Ins > enable 'Use VST3 Plug-In System Folders' and press Rescan."
