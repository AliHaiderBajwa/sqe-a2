# REPORT_TAIMOOR — Sections 1, 8, 9, 11 (for Ali to merge into REPORT_DRAFT.md)
Author: Taimoor Khalid 24i3052. Source of every claim: repo files / evidence / logs cited inline.
No PX4 or test code written; no numbers invented; UNVERIFIED items marked.

## Section 1 — Introduction + testing approach
We performed white-box structural testing of three selected PX4 v1.17.0 production areas,
chosen from direct source reading: `src/lib/hysteresis/hysteresis.cpp` (boolean state with
asymmetric time delays, GTest unit), `src/lib/pid/PID.cpp` (control-loop PID with saturation,
anti-windup and degenerate-input guards, GTest unit), and `src/lib/battery/battery.cpp`
(SoC fusion, RLS resistance estimation, warning/fault classification, remaining-time
estimation, GTest functional). Battery is additionally the MC/DC component because its
warning/fault outputs drive failsafe behavior. Obligations: 100% statement and 100%
decision/branch over this scope where practically achievable with every residual gap
individually justified, plus MC/DC independence pairs for the applicable Battery compound
decisions (L130 4-cond, L327–328, L370–371, L375, L377–378) with each condition and each
decision showing both outcomes in the workbook matrix. Upstream tests were read and run for
conventions and baseline only; all 51 claimed tests (11 Hysteresis + 29 Battery + 11 PID)
are student-authored and proven by run logs and the reconstructing patch. Production
behavior was never changed to gain coverage; the diff contains only test registrations
plus three new test files.

## Section 8 — Findings / limitations
- Failed/blocked tests: during development 2 Battery RLS-gate tests failed first (per PROGRESS_LOG;
  REPORT_DRAFT describes a third PID NaN-expectation fix — count UNVERIFIED pending Ali's
  confirmation). The Battery failures were test-side expectation errors on the L130 gate, which
  fires only pre-init with connected + IR-flag-set + n_cells>0; the expectations were corrected
  and production code was untouched. The PID case, if confirmed, was expecting 0.0 for +Inf
  feedback where IEEE semantics give NaN while the integral guard itself held.
- Confirmed defects: none. No reproducible production defect is claimed; the suite is green
  because the covered logic met its specification on the exercised paths, and no failure was
  manufactured.
- Coverage gaps (full table in COVERAGE_GAP_ANALYSIS.md): battery.cpp 230/231 lines, sole line
  gap L72 `PARAM_INVALID` defensive log, reachable only via corrupted param store (fault
  injection needed); hysteresis 3 sub-arcs at L77/L83 contradict the L75 entry guard
  (provably unreachable); B-D9 c1 `!finite(avg)` subsumed by the B-D10 finite guard
  (investigated, concrete); B-D10 c2 (armed + non-finite current) is a REAL untested outcome
  (`BRDA 375,0,3 = 0`) needing an armed+NaN probe; vehicle_status copy-fail and FW-transition
  sub-arcs (L356/L359) need uORB-failure-injection tests; ctor index `<1` partition needs a
  zero/negative-index test. Remaining ~70 battery sub-arcs are compiler-generated `e0`
  exception/unwind arcs with no source decision.
- Residual risks: only 3 of thousands of PX4 files analyzed; failsafe consumers, estimators,
  drivers, real-sensor noise, real-time scheduling, and RLS convergence on flight data are
  outside the evidence; FW remaining-time paths used synthetic phase stamps.
- Improvements: add the three named probe tests above via param/uORB stubbing; replay
  long-horizon current/voltage traces through `estimateStateOfCharge`; cover the FW gate with
  estimator-produced phase data in SITL; upstream the first-ever Battery functional test.

## Section 9 — Final quality judgment (300–400 words)
The structural evidence supports a narrow but strong claim: the three analyzed components behave as specified on every executable path reachable locally. Hysteresis reaches 22 of 22 lines with all reachable branches taken, including exact timer-expiry boundaries the upstream suite never probes. PID reaches 22 of 22 lines and 10 of 10 branch arcs, closing the non-finite-integral and derivative-history gaps the upstream baseline leaves open at 8 of 10. Battery, which had no upstream test and therefore zero baseline coverage, reaches 230 of 231 lines with all 19 functions covered, and independence pairs prove every atomic condition of the failsafe-critical warning, fault, and remaining-time gates can flip its decision, except cases proven unreachable by construction rather than by weak tests. Three student-test failures during development were each traced to wrong expectations, never to production defects, and no reproducible production defect is claimed; the suite is green because the covered logic met its specification on the exercised paths, and no failure was manufactured. Confidence is therefore high but strictly scoped: statement coverage is complete over the analyzed scope except one defensive param-corruption line, and every reachable decision shows both outcomes, so residual risk concentrates in individually justified gaps including the L72 corrupted-param log, three hysteresis arcs contradicting the entry guard, and the subsumed non-finite-average arm. Everything outside the scope remains unsupported: the failsafe consumers of these signals, the estimators feeding them, real-sensor noise, real-time scheduling, and the rest of the PX4 codebase, none of which this evidence addresses. In particular, resistance convergence was shown only on synthetic steps and fixed-wing remaining-time behavior only on synthetic phase stamps. The honest reading is that the selected business logic is structurally sound and deterministically verified at unit and functional levels, while any claim about PX4 as a whole would require system, simulation, and flight evidence far beyond this assignment. Methodologically, each test uses fresh objects, fixed synthetic timestamps, and hand-computed expectations, making every pass a reproducible white-box obligation discharge rather than anecdote.
<!-- WORD COUNT: 328 (python len(text.split())), within required 300-400. -->

## Section 11 — AI-assistance record
- Lead/Ali: AI agent (Sisyphus/OpenCode) used for repo navigation, build troubleshooting
  (space-in-path, Coverage build-type flag), test scaffolding, coverage measurement, workbook
  generation, report drafting and packaging. Assumptions it introduced: PID fallback matches
  Ashar's spec (verified by 11/11 PASS + 10/10 arcs); script transcription equals md tables
  (verified by 51/32 row reload checks); gap reachability claims (verified against `.info`
  BRDA + source guards; B-D10 c2 correctly flagged incomplete). Explore agents timed out on
  the PX4 tree; analysis fell back to direct grep/read. No production code AI-modified.
- Ashar Ahmed 24i3072: [PLACEHOLDER — add 5-line note: AI uses, assumptions, verification].
- Taimoor Khalid 24i3052: AI (OpenCode) used for cross-file consistency audit, coverage-gap
  classification from `.info`/BRDA data, report prose drafting, viva-bank derivation, packaging
  checklist; wrote no PX4 or test code. Assumption risk: line↔arc mapping from lcov numbering —
  mitigated by citing raw BRDA strings and marking UNVERIFIED where logs are absent
  (failure count, DOCX cover rendering). All numbers rechecked against evidence files.
- No raw chat transcripts are submitted, per the assignment brief.
