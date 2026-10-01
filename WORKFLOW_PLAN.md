# Assignment 02 — Master Workflow Plan
## Structural Testing and Coverage Analysis of PX4 Autopilot v1.17.0 (SE3002, 100 marks)

> **Living document.** Updated by Sisyphus after every work iteration without being asked.
> Source of truth for sequencing. Progress details live in `PROGRESS_LOG.md`.
> Report draft lives in `REPORT_DRAFT.md`. AI uses logged in `AI_ASSISTANCE_LOG.md`.
> Brief source: `Assignment02-SQE.docx.md` (227 lines, 6 pages + rubric).

---

## 1. Objective and grading strategy

**Core purpose (brief p.1):** derive defensible white-box tests from implementation of an unfamiliar production-scale system. No pre-selected class/function list — we choose, justify, test, and defend. Raw coverage % or AI-generated tests without our own analysis = fail.

**Coverage goal:** maximum practically achievable structural coverage of *meaningful business/control logic in our analyzed scope* — target 100% statement + 100% decision/branch for that scope. Every remaining gap must be individually identified, investigated, and technically justified with evidence. `tests_coverage` (gcov/lcov) or `llvm-cov` if platform requires. Screenshots alone insufficient — submit generated HTML/machine-readable reports.

**MC/DC (focused, not repo-wide):** one safety-/mission-/control-critical class or cohesive component, justified as critical. Define the critical behaviour, then MC/DC on the **non-trivial compound decisions that implement/govern that behaviour**: atomic conditions + independence pairs proving each condition's independent effect. For each such decision also show each atomic condition T+F and overall decision T+F in the workbook matrix (no separate condition-coverage %).

**Rubric mapping (where marks actually come from):**

| Phase | Rubric component | Marks | What wins marks |
|---|---|---|---|
| Phase 2 | Part 1 — analysis + test basis | 20 | Correct baseline+env, non-trivial scope, accurate deps/decisions/state, right test level per target |
| Phase 3 | Part 2 — derivation + MC/DC | 30 | Statement/branch obligations → tests, justified critical component, correct atomic conditions + independence pairs, edge/boundary/error/state cases, prod-logic→test traceability |
| Phase 4 | Part 3A — implementation+execution | 15 | Own tests compile+run, correct GTest unit/functional/SITL choice, deterministic setup-run-check, clean integration + patch |
| Phase 4 | Part 3B — coverage + gaps | 20 | Authentic baseline+final evidence, max-achievable pursued, gaps precisely justified, MC/DC demonstrated |
| Phase 5 | Part 4 — findings + judgment | 15 | Failures investigated before defect claims, defects reproducible w/ location+repro, residual risk stated, scoped 300–400w judgment, no overclaim |

**Viva deduction rule applies across all components.** Any member may be asked to: build/run a test, locate the prod decision, list atomic conditions, explain an independence pair, justify test level, interpret a gap, predict coverage loss if a test is removed. Mitigation: keep scope small enough that *every member understands every test* (prefer 2–4 files, one MC/DC component).

---

## 2. Environment baseline (recorded 2026-09-30, for Part 1 report §2)

- OS: Linux Mint 22.3 (Ubuntu 24.04 base, codename `zena`), x86_64, native Linux → **PX4 Ubuntu dev route** (no WSL needed).
- Toolchain found: gcc/g++ 13.3.0, Python 3.12.3, git 2.43.0. Disk free ~140 GB (PX4 recursive clone + build needs ~10–15 GB — OK).
- **Gap: `cmake` NOT installed.** Must install before any PX4 build (`sudo apt install` per PX4 docs + `Tools/setup/ubuntu.sh`).
- PX4 checkout: **not present** (checked `~/PX4-Autopilot` and workspace copy — absent).
- Commit hash / tag: **TBD — fill immediately after clone** (`git rev-parse HEAD` on `v1.17.0` recursive clone).

---

## 3. Phase plan with exit criteria

### Phase 0 — Setup + tracking (this turn) ✅ in progress
- [x] Read brief thoroughly. [x] Env audit. [x] Workspace dirs (`workbook/ evidence/{baseline,final,logs}/ patch/ report_assets/`).
- [ ] This plan + `REPORT_DRAFT.md` + `PROGRESS_LOG.md` + `AI_ASSISTANCE_LOG.md` created.
- Exit: all four md files exist; next command known.
- **Next command (Phase 1 start):** install cmake + PX4 prereqs, then recursive clone v1.17.0.

### Phase 1 — Fixed baseline + local build (brief workflow steps 1–2)
1. `git clone --branch v1.17.0 --recursive https://github.com/PX4/PX4-Autopilot.git` inside workspace (excluded from submission; only diff submitted).
2. `git rev-parse HEAD` → record hash in report + logs. Verify tag.
3. Install toolchain: PX4 `Tools/setup/ubuntu.sh` (needs sudo), cmake, ninja, genromfs, lcov/gcov, gtest deps per docs.
4. Baseline build+test proof: `make tests` (or documented subset if full suite too heavy), one `TESTFILTER` run, `make tests_coverage` baseline capture → save raw output to `evidence/baseline/` + `evidence/logs/`.
5. **No production edits.** Only env setup.
- Exit: build+test evidence reproducible; hash+OS+compiler+commands recorded. Any env blocker documented as investigated (counts toward gap honesty).

### Phase 2 — Repository analysis + scope selection (workflow steps 3–4, Part 1)
1. Explore prod code; **exclude** pure GUI/presentation, generated code, test-only utils, trivial getters/setters/wrappers — these must not inflate scope.
2. Shortlist 2–4 non-trivial business/control files with: real decisions, boundary/error/state behavior, controllable deps. Selection criteria (in priority order): (a) deterministic + low deps → GTest unit (fastest coverage loop, easiest viva defence); (b) safety/mission relevance → MC/DC candidate; (c) reachable in local env without SITL if possible (SITL only if logic needs time/drivers/full FC context).
3. Candidate pool to evaluate after clone (not committed): math/control helpers, failsafe/geofence logic, battery/estimator checks, flight-mode transitions. Final pick driven by: decision density, dependency cost, MC/DC richness (≥2 non-trivial compound decisions in one cohesive component — trivial single-`&&` wrapper rejected by brief).
4. For each scoped item record (report prose/bullets, **no separate scope table**): file/class, responsibility/critical behaviour, key deps/state (params, uORB, time, mode), why non-trivial + included, PX4 test level + why.
5. MC/DC component: select ONE, justify criticality (safety/mission/control argument), define critical behaviour, enumerate applicable compound decisions.
- Exit: scope frozen + justified; test level per target chosen; decision/compound-decision understanding visible via scope prose + workbook skeleton.

### Phase 3 — Test derivation + workbook (workflow step 5, Part 2)
1. Derive obligations from source: every executable statement + both outcomes of each reachable decision in scope; boundary/invalid/error/state-transition cases wherever source implies them.
2. MC/DC derivation per applicable decision: list atomic conditions, design independence pairs (each condition flipped while others held to show outcome change), ensure each condition T+F and decision T+F covered; note short-circuit/state/param setup explicitly in test code + compact inventory note.
3. Build `workbook/*.xlsx` (≤2 sheets, Excel/Sheets → export xlsx):
   - Sheet 1 Test Inventory: Test ID, component/function, purpose/scenario, key controlled input/state, expected result, execution result (PASS/FAIL/BLOCKED only if true — never manufacture failures), structural target, test-file/ref.
   - Sheet 2 MC/DC Evidence: decision, atomic values, outcome, independence pair, condition demonstrated, source/test ref.
   - No scope/gap/defect/traceability sheets. No duplicated step-by-step procedures (test code is authoritative).
- Exit: every scope decision has ≥1 deriving test; every MC/DC decision has a complete matrix row set; workbook validates as xlsx.

### Phase 4 — Implementation, execution, coverage iteration (workflow steps 6–7, Part 3)
1. Implement own tests with justified mechanism per target (GTest unit default; functional if params/uORB needed; SITL only if justified). Deterministic setup–run–check; no order dependence. Include CMake/registration changes in patch. Zero prod-behavior change (any testability change separately justified + proven behavior-neutral).
2. Prove authorship: diff/patch vs v1.17.0 (`git diff`/`git format-patch`) in `patch/`; be able to explain derivation of every test (viva: coverage lost if test X removed).
3. Coverage loop: baseline capture → own-tests capture → add/refine tests while defensible coverage remains. Never stop at an arbitrary %. Save HTML/machine-readable reports to `evidence/final/`; state per-scope-item line + branch results in report prose/excerpts (no redundant % copy-table).
4. Gap rule per remaining item: exact statement/branch/condition → why unreachable/impractical locally → what env/strategy would cover it. "Hardware dependent" alone rejected; hard-to-test logic must not be excluded to inflate %.
5. Run `lsp_diagnostics`-equivalent (compiler warnings clean), full relevant `make tests` green, logs in `evidence/logs/`.
- Exit: suite compiles+runs from documented commands; final coverage at max-achievable with every gap evidenced; patch reconstructs submission without uploading repo.

### Phase 5 — Findings, judgment, packaging (workflow step 8, Part 4)
1. Investigate every FAIL/BLOCKED (setup, expected value, state, data, env, deps) before claiming any defect. Genuine defect → report prose only: location, repro, expected vs actual, test ID ref. No defect tool/table, no minimum count.
2. Gaps/limitations/residual risk + concrete improvements (levels, state setup, sim support, dep control, instrumentation, testability refactor).
3. Final quality judgment **300–400 words** (enforced in template): what evidence supports, what unsupported, confidence level, what blocks broader claims. Never "PX4 is high quality / fully tested".
4. Package per checklist + naming `<Roll1_Roll2_Roll3_Section>.<ext>` (roll numbers TBD — **need from user**): concise report (this draft → docx/pdf), own test files + CMake changes, git diff/patch, setup/run instructions with exact commands, baseline+final coverage evidence, execution logs, 1 workbook xlsx, brief AI record.
5. Viva rehearsal: each member runs one test, traces one decision, explains one independence pair.
- Exit: submission complete, reproducible, every member viva-ready.

---

## 4. Deliverables checklist (portal upload)

- [ ] Report (concise; group details, env, tag/commit, analysis+scope, approach, MC/DC selection+interpretation, coverage, defects, gaps, 300–400w judgment)
- [ ] Student-authored/modified test files + CMake/registration changes
- [ ] Git diff/patch vs v1.17.0
- [ ] Setup/run instructions (exact commands)
- [ ] Baseline + final coverage evidence (HTML/equivalent + excerpts; screenshots only as supplement)
- [ ] Test execution evidence (logs/output, not just source screenshots)
- [ ] Workbook `.xlsx` (≤2 sheets: Inventory, MC/DC)
- [ ] Brief AI-assistance record
- [ ] Naming `<Roll1_Roll2_Roll3_Section>.<ext>`

---

## 5. Risks and mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| `cmake`/PX4 prereqs missing; full `make tests` heavy on student machine | Baseline blocked | Install via `ubuntu.sh`; fall back to documented subset + TESTFILTER scoping; log everything |
| SITL-only logic untestable locally | Coverage gaps | Prefer GTest-unit-testable scope; SITL only if justified; gaps evidenced per rule |
| Scope too big → viva indefensible | Mark loss across rubric | Cap scope (2–4 files, 1 MC/DC component); every member owns every test |
| Upstream-test resemblance accusation | Integrity fail | Read/run upstream for patterns only; own derivation documented; clean diff proves authorship |
| Overclaim in judgment | Part 4 loss | Scoped language; word-count enforced; residual risk explicit |

---

## 6b. Team split (conflict-free by design, viva-shared knowledge)

- **Lead (+Sisyphus):** Hysteresis unit tests, Battery functional + MC/DC derivation, build/coverage infra, final integration + DOCX export.
- **Ashar:** PID unit tests ONLY (`src/lib/pid/PIDStructuralTest.cpp` + 1 CMake line). Full brief: `TEAM_BRIEF_ASHAR.md`.
- **Taimoor:** workbook xlsx, report prose (§§1/8/9/11), viva Q&A bank, packaging audit. Zero PX4 edits. Full brief: `TEAM_BRIEF_TAIMOOR.md`.
- Rule: all three study the Q&A bank — viva can question any member on anything. Awaiting: name↔roll mapping + section.

## 6. Information still needed from user (to unblock packaging/viva)

1. Group: 3 roll numbers + names + section (for filenames + report cover).
2. Due date (brief leaves blank) — to set iteration cadence.
3. Sudo/admin available on this Mint machine? (needed for `ubuntu.sh`).
4. Report output format preferred: DOCX only, or DOCX+PDF? (template maintained in `REPORT_DRAFT.md`, exported at end).
5. Any member OS differences (if evidence must reproduce on a second machine, state both toolchains).

*Plan version: v1 (2026-09-30). Next update after Phase 1 clone+hash.*
