---
phase: 04-polish-publication
plan: "03"
subsystem: packaging
tags: [versioning, packaging, distribution, plugin-list]
dependency_graph:
  requires: []
  provides: [DLL-version-resource, zip-packaging, plugin-list-manifest]
  affects: [MarkdownPreview.vcxproj, MarkdownPreview.rc, manifest.json, scripts/package.ps1]
tech_stack:
  added: []
  patterns: [VS_VERSION_INFO, ResourceCompile, Compress-Archive, Get-FileHash]
key_files:
  created:
    - MarkdownPreview/MarkdownPreview.rc
    - manifest.json
    - scripts/package.ps1
  modified:
    - MarkdownPreview/MarkdownPreview.vcxproj
decisions:
  - "FILEVERSION 1,0,0,0 in .rc matches manifest.json version 1.0.0 (4th segment ignored by Notepad++)"
  - "manifest.json lives at repo root (not inside zip) — used for plugin list PR submission"
  - "package.ps1 uses wildcard Compress-Archive (Join-Path $stage *) to prevent nested root in zip"
  - "SHA-256 printed to console by package.ps1 rather than auto-written to manifest.json — submitter must update manually after verifying zip integrity"
metrics:
  duration: "2 minutes"
  completed_date: "2026-04-10"
  tasks_completed: 2
  files_created: 3
  files_modified: 1
---

# Phase 04 Plan 03: Publish-Ready Packaging Summary

**One-liner:** DLL version resource (VS_VERSION_INFO 1.0.0.0), vcxproj ResourceCompile registration, Notepad++ plugin list manifest.json, and automated PowerShell zip packager with SHA-256 output.

## Tasks Completed

| Task | Name | Commit | Files |
|------|------|--------|-------|
| 1 | Create MarkdownPreview.rc, update vcxproj, create manifest.json | 6df8289 | MarkdownPreview/MarkdownPreview.rc, MarkdownPreview/MarkdownPreview.vcxproj, manifest.json |
| 2 | Create scripts/package.ps1 | 3f52a76 | scripts/package.ps1 |

## What Was Built

### Task 1: DLL Version Resource + Manifest

**MarkdownPreview/MarkdownPreview.rc** — Windows VS_VERSION_INFO resource embedded in the DLL at compile time. Uses `FILEVERSION 1,0,0,0` and `PRODUCTVERSION 1,0,0,0`. Block `040904B0` (US English + Unicode 1200 charset). String values include CompanyName, FileDescription, FileVersion, InternalName, OriginalFilename, ProductName, ProductVersion.

**MarkdownPreview/MarkdownPreview.vcxproj** — Added `<ItemGroup><ResourceCompile Include="MarkdownPreview.rc" /></ItemGroup>` immediately before the `<Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />` line. MSBuild invokes `rc.exe` automatically on `<ResourceCompile>` items — no additional build configuration needed.

**manifest.json** — Repo-root file (not included in zip) containing all Notepad++ plugin list PR fields: `folder-name`, `display-name`, `version`, `id` (placeholder for SHA-256), `repository`, `description`, `author`, `homepage`. The `id` and `repository` URLs must be updated once GitHub releases are created.

### Task 2: Automated Packaging Script

**scripts/package.ps1** — PowerShell 5.1 script that:
1. Reads `bin\x64\Release\` and `bin\x86\Release\` build outputs
2. Validates required files exist (`MarkdownPreview.dll`, `WebView2Loader.dll`, `assets/`)
3. Stages files into `$env:TEMP\MdPreview_stage_{arch}` with correct zip layout (DLLs at root, `assets/` as subdirectory)
4. Uses `Compress-Archive -Path (Join-Path $stage "*")` wildcard to prevent nested directory in zip
5. Outputs `dist/MarkdownPreview_v{Version}_x64.zip` and `dist/MarkdownPreview_v{Version}_x86.zip`
6. Prints SHA-256 of each zip (the value for the `"id"` field in plugin list JSON)
7. Cleans up temp staging directories

Usage: `.\scripts\package.ps1` (default v1.0.0) or `.\scripts\package.ps1 -Version "1.0.1"`

## Deviations from Plan

None — plan executed exactly as written.

## Known Stubs

- `manifest.json` `"id"` field: `"REPLACE_WITH_SHA256_FROM_PACKAGE_PS1"` — intentional placeholder. Must be replaced with actual SHA-256 of the x64 zip after running `package.ps1` against a Release build. Documented in the file.
- `manifest.json` `"repository"` and `"homepage"` URLs use `hbeni` GitHub username — must be updated to match actual repository URL before plugin list PR submission.

## Threat Flags

None — no new network endpoints, auth paths, file access patterns, or schema changes at trust boundaries introduced. package.ps1 reads only from hardcoded `bin\x64\Release\` and `bin\x86\Release\` paths.

## Self-Check: PASSED

- MarkdownPreview/MarkdownPreview.rc: FOUND
- MarkdownPreview/MarkdownPreview.vcxproj (ResourceCompile): FOUND
- manifest.json: FOUND
- scripts/package.ps1: FOUND
- Commit 6df8289: FOUND
- Commit 3f52a76: FOUND
