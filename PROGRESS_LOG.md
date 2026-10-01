# PROGRESS_LOG — Assignment 02 (living tracker, auto-updated by Sisyphus)

> Rule: newest entry on top. Every work iteration appends an entry AND updates `REPORT_DRAFT.md` + this log without being asked.

## 2026-10-01 — Identities finalized (Ali Haider Bajwa 24i3102, Taimoor Khalid 24i3052, Ashar Ahmed 24i3072, Section C SE); package renamed to `24i3102_24i3052_24i3072_C_*`, DOCX + submission_pkg rebuilt, RUN_INSTRUCTIONS.md restored after rebuild wipe.

## 2026-10-01 — Report complete, DOCX exported, submission package assembled
- `REPORT_DRAFT.md` §§6/8/9/10/11 filled; stale Ashar-pending refs replaced (PID fallback done: 11 PST tests, 10/10 arcs). §9 judgment 324 words (300–400 enforced, verified programmatically).
- DOCX: `24i3102_24i3052_24i3072_SECTION-TBD_Report.docx` (python-docx from draft). Workbook: `workbook/Workbook.xlsx` (51 + 32 rows, 2 sheets).
- Package `submission_pkg/` (1.4M): report DOCX, workbook xlsx, 3 student test files, full patch (928 insertions), run instructions, baseline + final `.info` coverage, HTML report, 5 run logs, AI log.
- Remaining external inputs: member names + section (cover/filenames), teammate PID file swap if delivered (re-measure on arrival).

## 2026-10-01 — Coverage milestone: battery 99.6%, hysteresis/PID 100% lines; tables written
- Final (upstream+student, `evidence/final/scope_coverage_branch.info` + html/): hysteresis.cpp 22/22 lines, 17/20 arcs (3 provably unreachable, L75-guarded); PID.cpp 22/22, 8/10 (2 pending Ashar's NaN/Inf tests); battery.cpp 230/231 lines (only L72 PARAM_INVALID defensive gap), 19/19 fns, arcs 170/248.
- Baseline (`evidence/baseline/scope_baseline_branch.info`): hysteresis/PID at upstream levels; battery.cpp ABSENT = 0% (no upstream test) → our contribution 0→99.6%.
- Test count: 11 Hysteresis + 29 Battery = 40 student tests green (normal + coverage builds). 2 early Battery failures were test-side expectation errors (fixed, prod untouched).
- Workbook inputs ready: `TEST_INVENTORY.md` (40 rows + PID placeholder), `MCDC_MATRIX.md` (H-D5/H-D7/B-D4/B-D8/B-D9/B-D10/B-D11 pairs + investigated gaps B-D9-c1, H-arcs, L72). Patch: `patch/full_reconstruction.patch` (766 lines).
- Pending external: Ashar PIDStructural file (brief sent) → closes PID arcs + adds rows; Taimoor xlsx + report prose + viva bank (briefs + tables ready for him).

## 2026-10-01 — Student tests green: 11 Hysteresis + 20 Battery (Ashar PID pending)
- `unit-HysteresisStructural`: 11/11 PASS (MC/DC quads H-D5/H-D7, exact-expiry boundaries, stamp semantics, asymmetric windows).
- `functional-BatteryStructural`: 20/20 PASS after 2 fix cycles (failures were wrong test-side expectations on the L130 RLS gate: gate fires only pre-init + connected + IR-flag-true; corrected base/pair design, code untouched — genuine white-box learning, logged).
- Baseline upstream runs saved: `evidence/baseline/unit-{Hysteresis,PID}_run.log` (7/7, 4/4). Student runs: `evidence/logs/student_*_run.log`.
- Diff so far: 1-line hysteresis CMake + 2 new test files (upstream tests untouched). Battery CMake +1 line.
- Coverage detour fixed: `PX4_CMAKE_BUILD_TYPE` is make-wrapper-only; direct cmake needs `-DCMAKE_BUILD_TYPE=Coverage` (verified "Coverage instrumentation enabled"). Rebuilding scoped targets instrumented; lcov next.
- Next: baseline-vs-final lcov on 3 scoped files → gap analysis → workbook inputs for Taimoor → Ashar PID file integration → packaging.

## 2026-09-30 — Team briefs issued (Ashar: PID tests, Taimoor: workbook/report/viva)
- `TEAM_BRIEF_ASHAR.md`: isolated PID work (new `PIDStructuralTest.cpp` + 1 CMake line), derivation obligations incl. NaN/Inf/FLT_EPSILON gaps, deliverable list. `TEAM_BRIEF_TAIMOOR.md`: 2-sheet workbook, report §§1/8/9/11, 20-Q viva bank, packaging audit, zero code edits.
- Still needed back: name↔roll mapping (24i3102/24i3052/24i3072) + section.
- Baseline scoped build restarted detached after harness SIGINT kill (`baseline_build_scoped2.log`); direct cmake/ninja path confirmed working (Makefile wrapper bypassed, documented as build quirk).

## 2026-09-30 — Workspace moved to space-free path; baseline build relaunched
- Root cause of `make tests` instant failure: PX4 Makefile uses `$(SRC_DIR)` unquoted in recipes → space in `sqe A2` breaks configure. Fix: moved workspace `sqe A2` → `sqe_A2` (git repo verified intact: same hash `d6f12ad`, tag v1.17.0, clean status after removing setup-downloaded Xtensa tarball). `/tmp/px4build` symlink refreshed.
- Clone verified complete: all 29 submodules initialized.
- Baseline build relaunched from new path (log `evidence/logs/baseline_build_test2.log`).

## 2026-09-30 — Phase 2 manual analysis: scope FROZEN (explore agents failed, direct tools used)
- All 3 explore agents timed out (30 min, PX4 tree too large) with zero partial output. Switched to direct grep/read; conventions + scope recovered manually.
- GTest recipe confirmed: `px4_add_unit_gtest(SRC XTest.cpp LINKLIBS lib)` → `unit-X` binary via ctest/`TESTFILTER` (`cmake/px4_add_gtest.cmake`); functional variant links `gtest_functional_main` + px4_layer/uORB/params. Subclass-exposes-protected pattern (cf. `AdsbConflictTest`) is the template for Battery tests.
- Scope frozen: hysteresis.cpp (unit) + PID.cpp (unit) + battery.cpp (functional, also the MC/DC component). Excluded with reason: SlewRate (branch-free), circuit_breaker (superficial single-&&), GUI/generated/test-only. Report §§3/5 updated.
- Setup4 still running (locale generation stage); clone sim submodules still fetching (non-blocking).
- Next: setup completion check → `make tests` configure + baseline build of scoped targets → baseline coverage capture.

## 2026-09-30 — Setup retry via space-free symlink; explore agents launched
- `ubuntu.sh` failed at pip step: workspace path contains a space (`sqe A2`) which breaks its unquoted `requirements.txt` path. Fix: `/tmp/px4build` symlink → rerun in progress (ARM toolchain debs installing). Verdict on pip step pending.
- 3 background explore agents fired: (a) GTest conventions, (b) unit-testable scope shortlist, (c) MC/DC-critical recommendation. Awaiting results.
- Clone of sim submodules still finishing in background (non-blocking; core complete).

## 2026-09-30 — Phase 1 partial: toolchain in, baseline cloned + hash recorded
- Installed cmake 3.28.3, ninja 1.11.1, lcov 2.0 via apt (sudo). Full PX4 prereqs (`ubuntu.sh`) still to run.
- Recursive clone v1.17.0 in progress (1.3 GB; sim submodules still fetching in background, non-blocking). Core repo complete: `src/lib`, `src/modules`, `matrix` submodule present.
- Baseline pinned: tag `v1.17.0`, hash `d6f12ad1c4f70ad3230afd7d86e971421e02fef4`. Report §2 updated.
- Next (parallel): PX4 `ubuntu.sh` in background; 3 explore agents for (a) GTest conventions, (b) unit-testable scope candidates, (c) MC/DC-critical candidates.

## 2026-09-30 — User decisions recorded (ASAP mode)
- Group rolls: 24i3102, 24i3052, 24i3072 (names + section still needed for cover/filenames).
- Due date: ASAP, no fixed date — maximum pace, Phase 1 starts immediately.
- Sudo: available. Report export: DOCX only.
- Next: cmake/ninja/lcov install → recursive clone v1.17.0 → hash → baseline build/test.

## 2026-09-30 — Phase 0: brief absorbed, env audited, workspace + templates created
- Read `Assignment02-SQE.docx.md` fully (227 lines): Parts 1–4, workflow 1–8, workbook 2-sheet limit, 100/100 coverage target over selected scope, focused MC/DC, viva rule, submission checklist + naming.
- Env audit: Linux Mint 22.3 (Ubuntu 24.04 base) x86_64, gcc/g++ 13.3.0, Python 3.12.3, git 2.43.0, ~140 GB free; **cmake missing**; PX4 not cloned. Native-Linux PX4 route applies.
- Created dirs: `workbook/ evidence/{baseline,final,logs}/ patch/ report_assets/`; files: `WORKFLOW_PLAN.md` (v1), `REPORT_DRAFT.md` (template v1), `PROGRESS_LOG.md`, `AI_ASSISTANCE_LOG.md`.
- Decision: cap scope to 2–4 files + 1 MC/DC component so all 3 members stay viva-ready.
- Next: install cmake + PX4 prereqs (`Tools/setup/ubuntu.sh`), recursive clone v1.17.0, record hash, baseline build/test/coverage. Blocked on: user confirms sudo availability + group roll numbers/names/section + due date (see plan §6).
- Report updated: template v1 covers all checklist sections; §§2–10 hold TBDs for Phase 1/2 data.

---
