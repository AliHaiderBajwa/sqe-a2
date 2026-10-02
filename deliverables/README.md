# Deliverables — SE3002 Assignment 02 (Google Classroom upload set)

Group: Ali Haider Bajwa (24i3102), Taimoor Khalid (24i3052), Ashar Ahmed (24i3072) — Section C (SE).
SUT: PX4-Autopilot v1.17.0 @ `d6f12ad1c4f70ad3230afd7d86e971421e02fef4`.
Naming: `<Roll1_Roll2_Roll3_Section>.<ext>` = `24i3102_24i3052_24i3072_C.*`.

## Checklist cross-reference (brief Submission Checklist, p.5)

| # | Required item | File(s) in this folder |
|---|---|---|
| 1 | Concise report (group, env, tag/commit, scope, approach, MC/DC, coverage, defects, gaps, 300–400w judgment) | `24i3102_24i3052_24i3072_C_Report.docx` |
| 2 | Student test sources + CMake/test-registration changes | `HysteresisStructuralTest.cpp`, `PIDStructuralTest.cpp`, `BatteryStructuralTest.cpp` (unprefixed: build paths `src/lib/*/...` must be preserved to apply); CMake changes (+1/+1/+2 lines) inside the patch |
| 3 | Git diff/patch vs v1.17.0 (reconstruct without uploading repo) | `24i3102_24i3052_24i3072_C_Patch.patch` (`git apply` clean on tag checkout) |
| 4 | Setup/run instructions (exact commands) | `RUN_INSTRUCTIONS.md` |
| 5 | Baseline + final coverage (HTML + machine-readable) | `baseline_scope_coverage_branch.info`, `final_scope_coverage_branch.info`, `final_coverage_html/` |
| 6 | Test execution evidence (logs, not screenshots) | `unit-Hysteresis_run.log` (7/7), `unit-PID_run.log` (4/4), `student_HysteresisStructural_run.log` (11/11), `student_PIDStructural_run.log` (17/17), `student_BatteryStructural_run.log` (33/33) |
| 7 | Testing workbook, ≤2 sheets | `24i3102_24i3052_24i3072_C_Workbook.xlsx` (Test Inventory 61 rows + MC/DC Evidence 32 rows) |
| 8 | Brief AI-assistance record | `AI_ASSISTANCE_LOG.md` |

## Naming notes (justified exceptions)

- The three test `.cpp` files keep upstream build paths (`src/lib/hysteresis|pid|battery/…`); prefixing them would break the build and the patch. Their CMake registrations are in the patch.
- Evidence internals (`.info`, run logs, `final_coverage_html/`) keep tool-generated names so HTML cross-links and log references stay intact; `RUN_INSTRUCTIONS.md` / `AI_ASSISTANCE_LOG.md` keep canonical names for the same reason. All top-level deliverables follow the roll-number convention.
