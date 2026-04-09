---
phase: 02-core-preview
plan: GAP
type: execute
wave: 1
depends_on: []
files_modified:
  - MarkdownPreview/src/PreviewPanel.cpp
autonomous: true
requirements: [REND-01, THME-01, THME-02, THME-03]
gap_closure: true

must_haves:
  truths:
    - "Editing a .md file does not crash Notepad++ regardless of file length"
    - "Custom CSS from the config directory is applied to the preview background"
    - "Dark mode theme is applied after WebView2 finishes loading (not silently dropped)"
  artifacts:
    - path: "MarkdownPreview/src/PreviewPanel.cpp"
      provides: "Fixed getCurrentText(), fixed CSS path, theme replay in NavigationCompleted"
      contains: "LRESULT len"
  key_links:
    - from: "PreviewPanel::getCurrentText"
      to: "SCI_GETTEXT allocation"
      via: "LRESULT len guard before size_t cast"
      pattern: "LRESULT len"
    - from: "PreviewPanel::renderMarkdown"
      to: "custom.css file open"
      via: "m_configPath instead of APPDATA env var"
      pattern: "m_configPath"
    - from: "NavigationCompleted callback"
      to: "setTheme"
      via: "m_isDark replay after applyInitialZoom"
      pattern: "m_isDark"
---

<objective>
Close two UAT-identified bugs in PreviewPanel.cpp: a heap buffer overrun that crashes
Notepad++ on every .md keystroke, and two related CSS/theme bugs that prevent custom
styling and dark mode from taking effect.

Purpose: Restore the Phase 2 success criteria — live preview works without crashes and
respects user theming choices.

Output: Updated PreviewPanel.cpp with three targeted fixes, no new files.
</objective>

<execution_context>
@C:/Users/barnardh/source/repos/MarkdownPreview/.claude/get-shit-done/workflows/execute-plan.md
@C:/Users/barnardh/source/repos/MarkdownPreview/.claude/get-shit-done/templates/summary.md
</execution_context>

<context>
@.planning/PROJECT.md
@.planning/ROADMAP.md
@.planning/STATE.md
@.planning/phases/02-core-preview/02-HUMAN-UAT.md
</context>

<tasks>

<task type="auto">
  <name>Task 1: Fix getCurrentText() buffer overrun (Gap 1 — blocker)</name>
  <files>MarkdownPreview/src/PreviewPanel.cpp</files>
  <action>
    Target: PreviewPanel::getCurrentText(), lines 425-429.

    Current code (BROKEN):
      int len = static_cast<int>(::SendMessage(hSci, SCI_GETLENGTH, 0, 0));
      std::string utf8Text(static_cast<size_t>(len) + 1, '\0');
      ::SendMessage(hSci, SCI_GETTEXT, static_cast<WPARAM>(len + 1), ...);
      utf8Text.resize(static_cast<size_t>(len));

    Problem: SCI_GETLENGTH returns LRESULT (64-bit signed). Casting to int truncates any
    value above INT_MAX (~2 GB), producing a negative int. The static_cast<size_t> of a
    negative int wraps to ~18 EB, causing std::string to throw std::bad_alloc or, if
    memory is somehow available, SCI_GETTEXT writes beyond the buffer. Either path crashes.

    Fix — replace those four lines with:
      LRESULT len = ::SendMessage(hSci, SCI_GETLENGTH, 0, 0);
      if (len < 0) return L"";   // guard: should never happen but be safe
      std::string utf8Text(static_cast<size_t>(len) + 1, '\0');
      ::SendMessage(hSci, SCI_GETTEXT, static_cast<WPARAM>(static_cast<size_t>(len) + 1),
          reinterpret_cast<LPARAM>(utf8Text.data()));
      utf8Text.resize(static_cast<size_t>(len));

    The variable type change from int to LRESULT means the two subsequent SendMessage
    calls (SCI_GETTEXT WPARAM and utf8Text.resize) also need their casts updated to go
    through size_t, not int. The MultiByteToWideChar calls below line 432 are unaffected —
    they receive utf8Text.size() which is already size_t.

    Do NOT change anything else in the function body.
  </action>
  <verify>
    <automated>
      Build the solution in Visual Studio (msbuild MarkdownPreview.sln /p:Configuration=Debug /p:Platform=x64)
      and confirm zero errors and zero warnings related to PreviewPanel.cpp.
      Then manually load the DLL in Notepad++ (or confirm the build succeeds as a proxy
      since the crash reproduces on every .md keystroke — a successful build with the
      correct LRESULT type closes the truncation path).
    </automated>
  </verify>
  <done>
    - `int len` on line 425 is gone; replaced by `LRESULT len`.
    - Guard `if (len &lt; 0) return L"";` is present immediately after the SendMessage call.
    - The WPARAM for SCI_GETTEXT is cast through size_t, not int.
    - Build produces zero errors.
  </done>
</task>

<task type="auto">
  <name>Task 2: Fix custom CSS path and dark-mode replay (Gap 2 — major)</name>
  <files>MarkdownPreview/src/PreviewPanel.cpp</files>
  <action>
    Two sub-fixes in PreviewPanel.cpp, both strictly localized.

    --- Fix A: Custom CSS path (lines 476-496) ---

    Current code reads APPDATA from the environment and constructs a hardcoded path:
      wchar_t appData[MAX_PATH] = {};
      DWORD ret = ::GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);
      if (ret > 0 && ret < MAX_PATH) {
          std::wstring cssPath = std::wstring(appData)
              + L"\\Notepad++\\plugins\\config\\MarkdownPreview\\custom.css";
          ...
      } else {
          j["customCss"] = nullptr;
      }

    Problem: m_configPath (a std::wstring member, already populated via setConfigPath()
    during onNppReady) holds the exact path returned by NPPM_GETPLUGINSCONFIGDIR — e.g.,
    C:\Users\user\AppData\Roaming\Notepad++\plugins\config\MarkdownPreview. The hardcoded
    path may differ from the actual config dir Notepad++ uses, so the file is not found.

    Fix — replace the entire APPDATA block (the outer braces, GetEnvironmentVariableW call,
    conditional, and all nested code) with:
      {
          std::wstring cssPath = m_configPath + L"\\custom.css";
          std::ifstream cssFile(cssPath, std::ios::in | std::ios::binary);
          if (cssFile.is_open()) {
              std::string cssContent((std::istreambuf_iterator<char>(cssFile)),
                                      std::istreambuf_iterator<char>());
              cssFile.close();
              if (!cssContent.empty()) {
                  j["customCss"] = cssContent;
              } else {
                  j["customCss"] = nullptr;
              }
          } else {
              j["customCss"] = nullptr;
          }
      }

    Remove the GetEnvironmentVariableW call and the wchar_t appData array entirely — they
    are no longer needed. Keep the comment on line 473-474 but update it to reference
    m_configPath instead of %APPDATA%.

    --- Fix B: Dark-mode replay in NavigationCompleted (lines 330-342) ---

    Current NavigationCompleted callback (inside the WebView2 controller completion lambda):
      [this](...) -> HRESULT {
          applyInitialZoom(m_zoomLevel);
          if (!m_pendingFilePath.empty()) {
              std::wstring pending = m_pendingFilePath;
              m_pendingFilePath.clear();
              renderMarkdown(pending);
          }
          return S_OK;
      }

    Problem: setTheme(isDark) is called in onNppReady() before WebView2 exists. The guard
    at line 505-507 (`if (!m_webview || !m_webview2Initialized) return;`) silently drops
    it. The NavigationCompleted callback replays a pending render but never replays the
    stored m_isDark value, so dark mode never reaches the JS side.

    Fix — insert a theme replay call immediately after applyInitialZoom():
      applyInitialZoom(m_zoomLevel);
      // Replay stored theme: setTheme() may have been called before WebView2 was ready.
      setTheme(m_isDark);
      if (!m_pendingFilePath.empty()) { ... }

    setTheme() already stores m_isDark = isDark before the guard check, so calling it here
    with the stored m_isDark value correctly replays whatever theme was set (even if it was
    the default false). This mirrors the existing pattern used for pending render replay.

    Do NOT change setTheme() itself or anything outside the NavigationCompleted lambda.
  </action>
  <verify>
    <automated>
      Build: msbuild MarkdownPreview.sln /p:Configuration=Debug /p:Platform=x64 — zero errors.
      Manual smoke test (Notepad++ loaded with the rebuilt DLL):
        A) Place custom.css at %APPDATA%\Notepad++\plugins\config\MarkdownPreview\custom.css
           containing `body { background: pink !important; }`. Open a .md file.
           Expected: preview background is pink.
        B) With Notepad++ in dark mode, open a .md file.
           Expected: preview uses dark theme (dark background), not white.
    </automated>
  </verify>
  <done>
    - No GetEnvironmentVariableW or wchar_t appData[] in the renderMarkdown CSS block.
    - cssPath is constructed as m_configPath + L"\\custom.css".
    - NavigationCompleted callback calls setTheme(m_isDark) after applyInitialZoom().
    - Build produces zero errors.
    - Custom CSS pink background and dark mode are both applied on file open.
  </done>
</task>

<task type="checkpoint:human-verify" gate="blocking">
  <what-built>
    Both bug fixes applied to PreviewPanel.cpp and compiled successfully:
    1. getCurrentText() uses LRESULT len with a negative-value guard (no more crash).
    2. renderMarkdown() uses m_configPath for custom.css lookup (file found, CSS applied).
    3. NavigationCompleted replays m_isDark via setTheme() (dark mode applied on load).
  </what-built>
  <how-to-verify>
    1. Build and copy the updated MarkdownPreview.dll to Notepad++'s plugins folder.
    2. Open Notepad++ and open any .md file.
       Expected: Notepad++ does NOT crash. Preview renders the file content.
    3. Type some text in the .md file. Expected: preview updates without crash.
    4. Place custom.css (body { background: pink !important; }) at
       %APPDATA%\Notepad++\plugins\config\MarkdownPreview\custom.css, then switch to the
       .md file. Expected: preview background turns pink.
    5. Enable dark mode in Notepad++ (Settings > Preferences > Dark Mode), reopen the
       .md file. Expected: preview background is dark, not white.
  </how-to-verify>
  <resume-signal>Type "approved" if all three checks pass, or describe which check failed.</resume-signal>
</task>

</tasks>

<threat_model>
## Trust Boundaries

| Boundary | Description |
|----------|-------------|
| Editor text → C++ buffer | Scintilla returns raw bytes via SCI_GETTEXT; length must be validated before allocation |
| Filesystem → CSS loader | custom.css content is user-supplied and injected into JSON; must be handled by nlohmann (already done) |

## STRIDE Threat Register

| Threat ID | Category | Component | Disposition | Mitigation Plan |
|-----------|----------|-----------|-------------|-----------------|
| T-GAP-01 | Denial of Service | getCurrentText LRESULT truncation | mitigate | Change int to LRESULT + guard len &lt; 0 before allocation (this fix) |
| T-GAP-02 | Tampering | custom.css injected via nlohmann j["customCss"] | accept | nlohmann escapes all string content before JSON serialization; no raw concatenation |
| T-GAP-03 | Information Disclosure | cssPath uses m_configPath (no env var expansion) | accept | m_configPath is set by Notepad++ via NPPM_GETPLUGINSCONFIGDIR; no user-controlled input |
</threat_model>

<verification>
- Build succeeds with zero errors and zero new warnings in PreviewPanel.cpp
- `LRESULT len` appears at the corrected line; `int len = static_cast<int>(` is gone from getCurrentText()
- `m_configPath` appears in the CSS block; `GetEnvironmentVariableW` is gone from renderMarkdown()
- `setTheme(m_isDark)` appears inside the NavigationCompleted lambda after `applyInitialZoom()`
- Human UAT gaps 1 and 2 resolved (crash gone, CSS/dark mode applied)
</verification>

<success_criteria>
- Notepad++ does not crash when editing a .md file of any length
- Custom CSS placed in the plugin config directory is applied to the preview
- Dark mode activates in the preview immediately when a .md file is opened in dark-mode Notepad++
- All three changes fit in a single focused edit to PreviewPanel.cpp; no other files modified
</success_criteria>

<output>
After completion, create `.planning/phases/02-core-preview/02-GAP-SUMMARY.md` following
the standard summary template. Record:
- The three exact line ranges changed and what they became
- Build result (zero errors confirmed)
- UAT gap status: Gap 1 (crash) closed, Gap 2 (CSS + dark mode) closed
</output>
