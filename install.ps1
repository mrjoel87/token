# Installs "SMSW TOTEM.vst3" into the standard Windows VST3 folder and removes the old TOKEN.vst3
# (both share the same plugin ID, so only one may be installed). Run from an Administrator PowerShell.
param([string]$Config = "Release")
$ErrorActionPreference = "Stop"

$src = Join-Path $PSScriptRoot "build\TOKEN_artefacts\$Config\VST3\SMSW TOTEM.vst3"
$dst = "C:\Program Files\Common Files\VST3"

if (-not (Test-Path $src)) { throw "Not built yet: $src (run .\build.ps1 first)" }

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) { throw "Please run this script from an Administrator PowerShell." }

$old = Join-Path $dst "TOKEN.vst3"
if (Test-Path $old) { Remove-Item -Recurse -Force $old; Write-Host "Removed old TOKEN.vst3" }

$target = Join-Path $dst "SMSW TOTEM.vst3"
if (Test-Path $target) { Remove-Item -Recurse -Force $target }
Copy-Item -Recurse -Force $src $dst
Write-Host "Installed to $target" -ForegroundColor Cyan
Write-Host "In Ableton Live: Preferences > Plug-Ins > Rescan."
