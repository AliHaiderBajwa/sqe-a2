# PACKAGING_CHECKLIST — SE3002 A2 submission audit (Taimoor)
Convention: `<Roll1_Roll2_Roll3_Section>.<ext>` = `24i3102_24i3052_24i3072_C.*`.
Checked 2026-10-01 against `Assignment02-SQE.docx.md:194-206` checklist. Paths relative to `sqe-a2/`.

| # | Required item | Verdict | Path / note |
|---|---|---|---|
| 1 | Report incl. group details, env, tag/commit, scope, approach, MC/DC, coverage, defects, gaps, 300–400w judgment | PRESENT (content audit: see AUDIT_REPORT F4) | `submission_pkg/24i3102_24i3052_24i3072_C_Report.docx` + root copy same name (46K each). Cover names/section rendering inside DOCX UNVERIFIED (binary) — open and eyeball before upload. Failure count 2-vs-3 must be locked (F4). |
| 2 | Workbook `.xlsx`, ≤2 sheets (Inventory + MC/DC) | PRESENT WITH FIX NEEDED | `submission_pkg/24i3102_24i3052_24i3072_C_Workbook.xlsx` (2 sheets, 51+32 rows). But Sheet2 lacks source-ref column + counts 2 gap rows as pairs (AUDIT F5/F6). Ship `workbook/Workbook_proposed.xlsx` (this delivery) after Ali confirms, renamed to the `..._C_Workbook.xlsx` convention. NEVER overwrite `workbook/Workbook.xlsx`. |
| 3 | Student test files (3 new) | PRESENT | `submission_pkg/{HysteresisStructuralTest.cpp,BatteryStructuralTest.cpp,PIDStructuralTest.cpp}`. Names MUST stay unprefixed (build paths `src/lib/*/…Test.cpp`) — justified naming exception. PID file is AI-drafted fallback, not Ashar's (see VIVA Q26). |
| 4 | CMake/test-registration changes | PRESENT | In-patch: `src/lib/hysteresis/CMakeLists.txt` +1, `src/lib/pid/CMakeLists.txt` +1, `src/lib/battery/CMakeLists.txt` +2. Corroborated by `patch/student_changes.diff`. Zero production edits. |
| 5 | Git diff/patch vs v1.17.0 | PRESENT, VERIFIED | `submission_pkg/24i3102_24i3052_24i3072_C_Patch.patch` + `patch/full_reconstruction.patch` (6 files, 51 TESTs). `git -C px4-autopilot apply --check` → clean on tag checkout. |
| 6 | Setup/run instructions (exact commands) | PRESENT | `submission_pkg/RUN_INSTRUCTIONS.md` — clone, hash check, apply, cmake+ninja, 5 binaries, lcov chain, expected 7/7·11/11·4/4·11/11·29/29 + 22/22·10/10·230/231. Hash + env match report. |
| 7 | Baseline coverage (HTML or equiv + machine-readable) | PRESENT (partial path note) | `submission_pkg/baseline_scope_coverage_branch.info`; full baseline also in `evidence/baseline/scope_baseline_branch.info` + `unit-{Hysteresis,PID}_run.log`. No baseline HTML — acceptable (`.info` is machine-readable); do not screenshot-substitute. |
| 8 | Final coverage (HTML + `.info`) | PRESENT | `submission_pkg/final_scope_coverage_branch.info` (identical to `evidence/final/scope_coverage_branch.info`, `diff -q` SAME) + `submission_pkg/final_coverage_html/` (index + battery/hysteresis/pid). Full gcov HTML also in `evidence/final/html/`. |
| 9 | Test execution evidence (logs, not just screenshots) | PRESENT | `submission_pkg/student_{HysteresisStructural,BatteryStructural,PIDStructural}_run.log` (11/11·29/29·11/11) + `unit-{Hysteresis,PID}_run.log` (7/7·4/4). Identical to `evidence/logs/` + `evidence/baseline/`. No failing-test logs preserved (failure count UNVERIFIED — F4). |
| 10 | AI-assistance record (brief, no transcripts) | PRESENT (needs 2 placeholders) | `submission_pkg/AI_ASSISTANCE_LOG.md` + `REPORT_TAIMOOR.md §11` (this delivery) with Ashar/Taimoor personal-note placeholders. No transcripts — correct per brief. |
| 11 | Naming convention | MOSTLY OK, 2 notes | Prefixed: Report DOCX ✓, Workbook ✓, Patch ✓. Unprefixed-but-justified: 3 test `.cpp` (build paths), `RUN_INSTRUCTIONS.md`, `AI_ASSISTANCE_LOG.md`, `.info`/logs/html (evidence internals). Stale `SECTION-TBD` name lives only in `PROGRESS_LOG.md:9` prose (F1) — not a file. Taimoor working docs (`AUDIT_REPORT.md`, `COVERAGE_GAP_ANALYSIS.md`, `REPORT_TAIMOOR.md`, `VIVA_BANK.md`, this file) are NOT submission files — do not upload as-is; Ali merges prose. |
| 12 | Agent artifacts excluded | CHECK | `submission_pkg/` is clean (no `.omo`/`.codegraph`). Repo root contains `.omo/` + broken `.codegraph -> /home/zexr/...` symlink — MUST NOT be copied into any upload zip; verify `.gitignore` covers them. |

## Must-fix before upload (from AUDIT_REPORT)
1. Lock failure count (F4) → patch §8 prose in DOCX source.
2. Fix PID authorship wording (F3) → "AI-drafted, lead-verified fallback".
3. Decide B-D10 c2: add armed+NaN test or submit with disclosed incomplete MC/DC (F10).
4. Rebuild DOCX + prefixed Workbook from corrected sources; re-verify cover page + filenames.
5. Open the DOCX once: cover rolls/section, pasted counts (51 rows, 324→328-word judgment — recount after merge).
