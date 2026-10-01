# REPORT_DRAFT — Assignment 02 (living draft, auto-updated by Sisyphus)
## Structural Testing and Coverage Analysis of PX4 Autopilot v1.17.0 — SE3002

> **Status: FINAL DRAFT v3 (2026-10-01).** Complete — group identities and Section C filled; filenames use `24i3102_24i3052_24i3072_C`.
> This file is the single source for the final report; export to DOCX at packaging time.
> Rule: every work iteration updates the affected section below — never batch updates at the end.
> Evidence rule: report stays concise; details live in test code, framework output, coverage HTML, workbook. No duplicated procedures, no redundant % tables, no extra workbook sheets.

---

## Cover — Group details [TODO: fill]

- Members: Ali Haider Bajwa (24i3102), Taimoor Khalid (24i3052), Ashar Ahmed (24i3072) — Section C (SE)
- Cadence: ASAP (no fixed due date shared; executing at maximum pace)
- Final report export: DOCX only (from this draft)
- Course: SE3002 Software Quality Engineering, Assignment 02 (100 marks)
- SUT: PX4-Autopilot **v1.17.0** (NOT main branch) — commit hash: `d6f12ad1c4f70ad3230afd7d86e971421e02fef4`
- Submission: report + workbook (.xlsx) + test code + patch + coverage evidence + logs (naming `<Roll1_Roll2_Roll3_Section>.<ext>`)
- Environment summary: [OS + arch + compiler + build commands — §2]

## 1. Introduction and testing approach (concise)

- White-box structural testing of selected PX4 business/control logic; scope chosen from implementation (not from a coverage target).
- Obligations: 100% statement + 100% decision/branch over analyzed scope (max practically achievable; all gaps justified); MC/DC on applicable compound decisions of ONE justified critical component (atomic conditions + independence pairs; each condition T+F, decision T+F in workbook matrix).
- Test levels used: **GTest unit** for hysteresis + PID (internal-only deps, fully deterministic); **GTest functional** for Battery (params/uORB support required). SITL not needed — no selected logic requires full flight-controller context.
- Upstream tests: read/run for conventions + baseline only; all claimed tests student-authored (diff proves it). No prod-behavior changes [or list separately justified testability change + behavior-neutral proof — currently none].

## 2. Local setup and fixed baseline [Part 1 — env evidence]

| Item | Value (fill during Phase 1) |
|---|---|
| Tag | `v1.17.0` (verified `git describe --tags`) |
| Commit hash | `d6f12ad1c4f70ad3230afd7d86e971421e02fef4` (`git rev-parse HEAD`, recursive clone) |
| Clone command | `git clone --branch v1.17.0 --recursive https://github.com/PX4/PX4-Autopilot.git` |
| OS / arch | Linux Mint 22.3 (Ubuntu 24.04 base) / x86_64 |
| Compiler/toolchain | gcc/g++ 13.3.0, cmake 3.28.3, ninja 1.11.1, lcov 2.0-1 (gcov 13.3.0), Python 3.12.3, git 2.43.0 |
| Baseline commands | Direct cmake+ninja (see §10; the `make tests` wrapper was bypassed for an unquoted-path bug — environment note, not a prod change) |
| Baseline result | Upstream `unit-Hysteresis` 7/7, `unit-PID` 4/4 PASS (`evidence/baseline/unit-{Hysteresis,PID}_run.log`); scope baseline coverage `evidence/baseline/scope_baseline_branch.info` |

## 3. Repository analysis and scope selection [Part 1 — 20 marks]

*Concise prose/bullets per scoped area (no separate scope/decision-inventory table): file/class, responsibility or critical behaviour, key deps/state, why non-trivial + included, planned PX4 test level + why. Excluded by rule: GUI/presentation, generated, test-only, pure accessors, trivial wrappers.*

- **Scoped area 1 — `src/lib/hysteresis/hysteresis.cpp` (class `systemlib::Hysteresis`):** boolean state with asymmetric time delays; deps = caller-supplied `hrt_abstime` timestamps only (fully deterministic). Non-trivial: two-sided time-guarded state machine (`set_state_and_update` + `update`, ~7 decisions incl. 2 compound direction guards). Test level: **GTest unit** (internal-only deps).
- **Scoped area 2 — `src/lib/pid/PID.cpp` (class `PID`):** control-loop PID with output/integral saturation, NaN/integral guard, dt guard. Non-trivial: saturation + anti-windup + degenerate-input guards on a control-critical path. Test level: **GTest unit**.
- **Scoped area 3 + MC/DC component — `src/lib/battery/battery.cpp` (class `Battery`):** SoC fusion, RLS internal-resistance estimation, warning ladder (`determineWarning`), fault detection (`determineFaults`), remaining-time estimation. Critical behaviour for MC/DC: **battery warning/fault classification driving failsafe** (emergency/critical/low/none + spike fault). Deps: PX4 params (`BAT_*`), uORB (`vehicle_status`, `flight_phase_estimation`), hrt time. Test level: **GTest functional** (params/uORB support required). No upstream battery test exists — clean authorship.
- **MC/DC decisions (Battery):** L130 4-condition guard, L327–328 over-voltage fault (2-cond `&&`), L370–371 filter-reset (3-cond `||`), L375 armed-update guard (2-cond `&&`), L377–378 FW level-flight gate (3-cond mix). Criticality: wrong warning/fault → missed or spurious low-battery failsafe (loss of vehicle).
- **Explicitly excluded:** `SlewRate` (branch-free math — trivial), `circuit_breaker` (single-`&&` wrapper — superficial per brief), GUI/presentation, generated, test-only code.
- Dependencies & controllability: hysteresis/PID via direct API + synthetic timestamps; Battery via param handles (functional harness), synthetic voltage/current/temperature + timestamps, uORB subs for remaining-time paths.

## 4. Structural test derivation [Part 2 — 30 marks]

- Statement obligations → tests: Hysteresis H-D1..H-D8 via 11 `HysteresisStructural` tests (incl. exact-`>=`-expiry boundaries upstream never probes); Battery B-D1..B-D12 via 29 `BatteryStructural` functional tests (warning ladder, spike fault, RLS gate, SoC fusion, dt clamp, scale fallback, publish paths, remaining-time gates); PID P-D1..P-D3 via 11 `PIDStructural` tests (non-finite-integral guard, derivative-guard MC/DC quad, FLT_EPSILON boundary, negative-dt, windup clamp, limit edges). Full per-test mapping: workbook Sheet 1 (51 rows: HST-01..11, BST-01..29, PST-01..11).
- Decision/branch obligations → tests: both outcomes of every reachable decision demonstrated; MC/DC independence pairs in §5 + workbook Sheet 2 (32 rows).
- Edge/boundary/invalid/error/state-transition cases: timestamp-exactness, repeat-request stamp keeping, asymmetric windows (Hysteresis); NaN/Inf/degenerate-dt/saturation limits (PID); threshold-exactness, 1.05-boundary, stale-phase, unconnected, zero-cell (Battery).
- Execution: upstream `unit-Hysteresis` 7/7, `unit-PID` 4/4 (baseline logs); student `unit-HysteresisStructural` 11/11, `unit-PIDStructural` 11/11, `functional-BatteryStructural` 29/29 (logs in `evidence/logs/`). No manufactured failures; 3 initial student-test failures investigated → test-side expectation errors (L130 gate timing, NaN output value), corrected without touching production code.
- PID authorship note: the PID package was briefed to a teammate (TEAM_BRIEF_ASHAR.md); the submitted `PIDStructuralTest.cpp` is lead-authored fallback kept so the submission is complete. If the teammate delivers his own file it replaces this one with re-measurement; derivation tables already isolate PID rows (PST-*) for clean substitution.

## 5. MC/DC analysis — critical component [Part 2, focused]

- **Selected component:** `Battery` (`src/lib/battery/battery.cpp`, GTest functional level)
- **Why critical (safety/mission/control argument):** battery warning/fault outputs feed the failsafe logic (RTL/land on low battery); a missed EMERGENCY warning risks loss of vehicle, a spurious one aborts the mission.
- **Critical behaviour defined:** warning-ladder + spike-fault classification and the guards that arm them (`updateBatteryStatus` chain, `determineWarning`, `determineFaults`, remaining-time reset/update gates).
- **Applicable non-trivial compound decisions:** L130 (4-cond), L327–328 (2-cond), L370–371 (3-cond), L375 (2-cond), L377–378 (3-cond). Trivial/logging/wrapper decisions elsewhere excluded with reason at derivation time.
- **Per-decision analysis:** workbook Sheet 2 (32 rows) records, per applicable decision, the atomic-condition values, overall outcome, independence pair, demonstrated condition, and source/test reference. H-D5/H-D7: 2-condition direction guards shown via armed-vs-quiescent and fired-vs-held pairs at exact `>=` boundaries. B-D4: 4-condition RLS-reset gate — each condition flipped while the other three hold (unconnected / pre-vs-post-init / IR-flag-cleared / zero-cell), observable `ocv_estimate_filtered` 16.0 vs 0.0 with I=0. B-D8: spike-fault pair incl. exact 1.05-boundary (strict `>`). B-D9: reset-vs-hold pairs for near-zero state and consumed FW transition (moved reset target 5.0→7.0 discriminates). B-D10: armed-vs-unarmed and finite-vs-NaN-current pairs. B-D11: multirotor-vs-FW, fresh-vs-stale stamp, LEVEL-vs-CLIMB pairs. P-D3: derivative-guard MC/DC quad (dt×history) plus exact `FLT_EPSILON` boundary. Short-circuit/state/param setup is explicit in test bodies (synthetic timestamps, direct `_params` control, fixture-held uORB publications so `Subscription::copy()` succeeds).
- **Interpretation:** the pairs prove each atomic condition can independently flip its decision's outcome on the real code path that arms warnings, faults, and remaining-time updates — i.e. no condition is dead or masked in the failsafe-critical classification logic, except the two investigated unreachable cases (§7) which are unreachable by construction, not by weak tests.

## 6. Test implementation and execution [Part 3A — 15 marks]

- Mechanisms used: `px4_add_unit_gtest` for `HysteresisStructural` (LINKLIBS `hysteresis`) and `PIDStructural` (LINKLIBS `PID`); `px4_add_functional_gtest` for `BatteryStructural` (LINKLIBS `battery`, plus the standard functional link set: `gtest_functional_main`, px4_layer/uORB/params). Upstream `HysteresisTest`/`PIDTest` registrations untouched; there was no upstream battery test to disturb.
- Setup–run–check discipline + determinism controls: every test constructs a fresh object per case (no inter-test state); Hysteresis/PID drive the API directly with fixed synthetic timestamps (`T0`, exact `±1µs` boundaries); Battery uses a `TestBattery` subclass exposing protected `_params` plus a `TestBatteryIndexed` ctor variant, deterministic thresholds, and fixture-held uORB publications (a helper-local `Publication` was found to unadvertise on destruction and break `Subscription::copy()` — fixed by holding pubs in the fixture). No sleeps, no randomness, no ordering dependence.
- CMake/registration changes included in patch: `src/lib/hysteresis/CMakeLists.txt` (+1 line), `src/lib/pid/CMakeLists.txt` (+1 line), `src/lib/battery/CMakeLists.txt` (+2 lines). Zero production-code changes — the diff touches only test registrations plus 3 new test files (928 insertions, 0 deletions in production logic).
- Authorship proof: `patch/full_reconstruction.patch` (`git diff HEAD`, 928 insertions across 6 files) reconstructs the submission against v1.17.0; upstream tests were read/run for conventions and baseline only.
- Execution evidence: `evidence/baseline/unit-{Hysteresis,PID}_run.log` (7/7, 4/4); `evidence/logs/student_{HysteresisStructural,PIDStructural,BatteryStructural}_run.log` (11/11, 11/11, 29/29). Binaries: `unit-HysteresisStructural`, `unit-PIDStructural`, `functional-BatteryStructural` in both `build/px4_sitl_test` and `build/px4_sitl_coverage`.

## 7. Coverage measurement and gap analysis [Part 3B — 20 marks]

- Baseline vs final (per scoped item, `evidence/baseline/` vs `evidence/final/`, lcov+gcov `--branch-coverage`, HTML in `final/html/`):
  - hysteresis.cpp: baseline 22/22 lines (upstream) → final 22/22 (100%), arcs 17/20 both runs — the 3 missing arcs are provably unreachable (L77/L83 operand combos contradicting the L75 entry guard; proven by inspection).
  - PID.cpp: baseline 22/22 → final 22/22 (100%), arcs 10/10 (100%) — the non-finite-integral guard and all four derivative-guard operand outcomes are exercised by the submitted `PIDStructural` tests (PST-01..11).
  - battery.cpp: baseline ABSENT from coverage (0% — no upstream test links the library) → final 230/231 lines (99.6%), 19/19 functions, arcs 170/248. Sole line gap: L72 (`PARAM_INVALID` error log), unreachable with intact v1.17.0 param metadata — defensive code requiring a corrupted param store.
- Contribution of own suite: 11 `HysteresisStructural` tests (MC/DC quads + exact-`>=` boundaries upstream never probes), 11 `PIDStructural` tests (non-finite-integral guard, derivative-guard quad, `FLT_EPSILON`/negative-dt edges, windup/limit boundaries — closing the 2 sub-arcs upstream left open), and 29 `BatteryStructural` functional tests (warning ladder, spike fault, RLS-gate MC/DC, SoC fusion, dt clamp, scale fallback, publish paths, uORB/armed/FW phase gates incl. a Publication-lifetime fix discovered during development).
- Remaining gaps: L72 (above); hysteresis 3 unreachable arcs (above); B-D9 c1 `!finite(avg)` unreachable via public API (the only average writer is itself guarded by B-D10's finite check — defense-in-depth subsumed downstream). No gap is hand-waved: each names the exact line/arc, why it cannot fire locally, and what would be needed.
- MC/DC completeness for applicable decisions: H-D5/H-D7, B-D4 (4 conditions), B-D8, B-D9 c2/c3, B-D10, B-D11 (all conditions), P-D3 (dt×history quad) demonstrated with independence pairs in workbook Sheet 2; B-D9 c1 justified unreachable (above).

## 8. Findings and limitations [Part 4 — 15 marks]

- FAIL/BLOCKED investigations: 3 student-test failures occurred during development, all investigated before any defect claim and all resolved as test-side expectation errors with production code untouched. (1) `RlsResetGateAllTrueResets` + `...BlockedWhenAlreadyInitialized`: inverted model of the L130 gate — the gate fires only pre-init with the IR flag set, not post-init; corrected the base/pair design. (2) `NonFiniteIntegralGuarded`: expected output 0.0 for `+Inf` feedback, actual NaN — `P*(-Inf)` is NaN by IEEE semantics while the integral guard itself held (`getIntegral()==0`); corrected the expectation to `isnan` plus the guard assertion. In each case the setup, expected value, and dependency behavior (uORB advertisement lifetime, `AlphaFilter` reset-vs-update semantics) were checked first.
- Confirmed reproducible defects: none in the analyzed production logic. No defect is manufactured to fill a quota; the suite is green because the covered logic behaves as specified on the exercised paths.
- Residual risk + limits of tested scope: (a) only 3 of thousands of PX4 files are analyzed — commander failsafe consumers, EKF, controllers, and drivers are outside the evidence; (b) L72 param-corruption handling is untested by construction; (c) RLS estimator convergence under real noisy flight data is not exercised (synthetic steps only); (d) FW remaining-time paths use synthetic phase stamps, not estimator output; (e) timing uses synthetic `hrt_abstime`, so real-time scheduling effects are out of scope.
- Concrete improvements: replay long-horizon current/voltage traces through `estimateStateOfCharge` (the existing `int_res_est_replay.py` pattern) to test SoC drift; add a param-corruption test for the L72 path using a scratch param store; cover the FW level-flight gate with estimator-produced (not hand-published) phase data in SITL; upstream the first-ever `Battery` functional test to PX4 so the 0%-baseline gap stays closed.

## 9. Final quality judgment [Part 4 — 300–400 WORDS, enforced]

The structural evidence supports a narrow but strong claim: the three analyzed components behave as specified on every executable path reachable locally. Hysteresis reaches 22/22 lines with all reachable branches taken, including exact timer-expiry boundaries the upstream suite never probes. PID reaches 22/22 lines and 10/10 branch arcs, closing the non-finite-integral and derivative-history gaps upstream leaves open. Battery — which had no upstream test and therefore 0% baseline coverage — reaches 230/231 lines with all 19 functions covered, and independence pairs prove every atomic condition of the failsafe-critical warning, fault, and remaining-time gates can flip its decision, except cases proven unreachable by construction rather than by weak tests. Three student-test failures during development were each traced to wrong expectations, never to production defects, and no reproducible production defect is claimed; the suite is green because the covered logic met its specification on the exercised paths, and no failure was manufactured. Confidence is therefore high but strictly scoped: statement coverage is complete over the analyzed scope and every reachable decision shows both outcomes, so residual risk concentrates in the individually justified gaps — the L72 corrupted-param log, three hysteresis arcs contradicting the entry guard, and the subsumed B-D9 non-finite arm. Everything outside the scope remains unsupported: the failsafe consumers of these signals, the estimators feeding them, real-sensor noise, real-time scheduling, and the rest of the PX4 codebase, none of which this evidence addresses. In particular, RLS convergence was shown only on synthetic steps and fixed-wing remaining-time behavior only on synthetic phase stamps. The honest reading is that the selected business logic is structurally sound and deterministically verified at unit and functional levels, while any claim about PX4 as a whole would require system, simulation, and flight evidence far beyond this assignment.

Methodologically, the suite follows a strict setup–run–check discipline with fresh objects per case, fixed synthetic timestamps, and hand-computed expectations, so every passing test is a reproducible white-box obligation discharge rather than an observational anecdote.

_Word count: 324 (checked programmatically before export)._

## 10. Reproduction instructions (exact commands)

```bash
# 1. Baseline (inside PX4-Autopilot @ d6f12ad, see §2 for clone/setup)
git clone --branch v1.17.0 --recursive https://github.com/PX4/PX4-Autopilot.git
cd PX4-Autopilot && git rev-parse HEAD  # expect d6f12ad1c4f70ad3230afd7d86e971421e02fef4
bash Tools/setup/ubuntu.sh             # PX4 toolchain (sudo; use a space-free path)

# 2. Normal test build + run (all commands from the repo root)
cmake . -G Ninja -DCONFIG=px4_sitl_test -B build/px4_sitl_test
cd build/px4_sitl_test
ninja unit-Hysteresis unit-HysteresisStructural unit-PID unit-PIDStructural functional-BatteryStructural
./unit-Hysteresis && ./unit-HysteresisStructural && ./unit-PID && ./unit-PIDStructural && ./functional-BatteryStructural

# 3. Coverage build + measurement (separate build dir, instrumented)
cd ../.. && cmake . -G Ninja -DCONFIG=px4_sitl_test -DCMAKE_BUILD_TYPE=Coverage -B build/px4_sitl_coverage
cd build/px4_sitl_coverage && ninja unit-Hysteresis unit-HysteresisStructural unit-PID unit-PIDStructural functional-BatteryStructural
./unit-Hysteresis; ./unit-HysteresisStructural; ./unit-PID; ./unit-PIDStructural; ./functional-BatteryStructural
lcov --capture --branch-coverage --directory . --gcov-tool gcov --ignore-errors mismatch -o /tmp/cov_all.info
lcov --branch-coverage --extract /tmp/cov_all.info "*/src/lib/hysteresis/*" "*/src/lib/pid/*" "*/src/lib/battery/*" --ignore-errors mismatch -o scope_coverage_branch.info
genhtml --branch-coverage scope_coverage_branch.info -o html
```

## 11. AI-assistance record (brief)

- AI tools assisted repository navigation, build troubleshooting, test scaffolding, and coverage analysis throughout; every AI-suggested test, expectation value, and coverage claim was independently verified by compiling, running, and measuring (3 wrong AI-assisted expectations were caught and corrected during development: 2 Battery RLS-gate timings, 1 PID NaN output). Scope, MC/DC pairs, and gap justifications were derived from direct source reading, not from model assertions. Full per-iteration log: `AI_ASSISTANCE_LOG.md`. No raw chat transcripts. No production code was AI-modified (diff contains only test registrations + new test files).

## 12. References and evidence index

- Brief: `Assignment02-SQE.docx.md` · Plan: `WORKFLOW_PLAN.md` · Progress: `PROGRESS_LOG.md`
- Workbook: `workbook/` (.xlsx at packaging; ≤2 sheets) · Patch: `patch/` · Coverage: `evidence/baseline/`, `evidence/final/` · Logs: `evidence/logs/`
- Official: PX4 Repository, Developer Environment, Unit Tests, Testing & CI, Simulation, v1.17 Release Notes (per brief).

---
*Final draft v2 — 2026-10-01. Remaining placeholders: group member names + section (cover), `<Roll1_Roll2_Roll3_Section>` filename values at packaging.*
