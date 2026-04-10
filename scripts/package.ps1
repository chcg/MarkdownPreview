#Requires -Version 5.1
<#
.SYNOPSIS
    Package MarkdownPreview plugin for Notepad++ plugin list distribution.
    Produces dist/MarkdownPreview_v{Version}_x64.zip and _x86.zip.
    Prints SHA-256 of each zip for use in pl.x64.json / pl.x86.json "id" field.

.PARAMETER Version
    Semantic version string, e.g. "1.0.0". Default: "1.0.0"

.EXAMPLE
    .\scripts\package.ps1
    .\scripts\package.ps1 -Version "1.0.1"
#>
param(
    [string]$Version = "1.0.0"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Resolve paths relative to repo root (script lives in scripts/ one level below root)
$repoRoot = Split-Path $PSScriptRoot -Parent
$binX64   = Join-Path $repoRoot "bin\x64\Release"
$binX86   = Join-Path $repoRoot "bin\x86\Release"
$distDir  = Join-Path $repoRoot "dist"

# Ensure dist/ directory exists
New-Item -ItemType Directory -Force -Path $distDir | Out-Null

foreach ($arch in @("x64", "x86")) {
    $binDir  = if ($arch -eq "x64") { $binX64 } else { $binX86 }
    $zipName = "MarkdownPreview_v${Version}_${arch}.zip"
    $zipPath = Join-Path $distDir $zipName

    Write-Host "`nPackaging ${arch}..."

    # Verify required files exist before staging
    $dll      = Join-Path $binDir "MarkdownPreview.dll"
    $loader   = Join-Path $binDir "WebView2Loader.dll"
    $assetsIn = Join-Path $binDir "assets"
    foreach ($required in @($dll, $loader, $assetsIn)) {
        if (-not (Test-Path $required)) {
            Write-Error "Required file/dir not found: $required`nBuild Release|${arch} first."
        }
    }

    # Stage into a temp directory -- use wildcard when zipping to put contents at root
    $stage = Join-Path $env:TEMP "MdPreview_stage_$arch"
    if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
    New-Item -ItemType Directory -Force -Path $stage | Out-Null

    # Copy DLLs to stage root (DLL must be at zip root per Plugin Admin requirement)
    Copy-Item $dll    (Join-Path $stage "MarkdownPreview.dll")
    Copy-Item $loader (Join-Path $stage "WebView2Loader.dll")

    # Copy assets/ subtree preserving directory structure
    Copy-Item $assetsIn (Join-Path $stage "assets") -Recurse

    # Remove existing zip if present (Compress-Archive fails if destination exists)
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }

    # IMPORTANT: use "$stage\*" (wildcard) not "$stage" -- prevents nested root dir in zip.
    # Plugin Admin requires MarkdownPreview.dll at zip root, not inside a subdirectory.
    Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zipPath

    # Compute SHA-256 -- this is the value for the "id" field in pl.x64/x86.json
    $hash = (Get-FileHash $zipPath -Algorithm SHA256).Hash.ToLower()
    Write-Host "  Output:  $zipPath"
    Write-Host "  SHA-256: $hash"
    Write-Host "  (Use this hash as the `"id`" field in the Notepad++ plugin list JSON)"

    # Clean up staging directory
    Remove-Item $stage -Recurse -Force
}

Write-Host "`nPackaging complete. Update manifest.json with the x64 SHA-256."
