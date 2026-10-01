# AI_ASSISTANCE_LOG — Assignment 02 (brief record, per brief requirement)

> Brief rule: identify material AI uses + important assumptions introduced; no raw chat transcripts.
> This log is updated each iteration; a summary is mirrored into `REPORT_DRAFT.md §11` at packaging.

| Date | Material use | Assumption introduced | Verified how |
|---|---|---|---|
| 2026-09-30 | Repository navigation/planning: parsed 227-line brief into phased plan, report template, workspace scaffolding | Env/toolchain facts taken from live `bash` audit (not model memory); scope candidates deferred until post-clone exploration | Audit commands re-runnable; scope TBD gated on actual source reads |
| 2026-10-01 | PID fallback tests + workbook/xlsx + report/DOCX/packaging: drafted `PIDStructuralTest.cpp` (11 tests) to Ashar's brief spec, generated `Workbook.xlsx` from inventory tables via script, wrote §§6/8/9/10/11 + judgment (324 words), exported DOCX via python-docx | Assumed fallback matches Ashar spec closely enough to swap; assumed script transcription == md tables (asserted 51/32 row counts) | PID 11/11 PASS + 10/10 arcs measured; xlsx re-loaded and row/header-checked; judgment word-counted programmatically |
| | | | |

## Standing assumptions to verify later
- PX4 v1.17.0 recursive clone + `ubuntu.sh` + `make tests`/`tests_coverage` behave per official docs on Mint 22.3 (Ubuntu 24.04 base) — verify in Phase 1, log deviations.
- No test code authored by AI yet; all future AI-suggested tests require human derivation check + independent coverage evidence before claiming.
