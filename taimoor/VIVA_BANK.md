# VIVA_BANK — SE3002 A2 defence Q&A (Taimoor, shared study material; updated by lead 2026-10-02 to post-Ashar/probe truth: 61 tests, PID 17/17, B-D10 c2 demonstrated)
SUT: PX4 v1.17.0 @ `d6f12ad1c4f70ad3230afd7d86e971421e02fef4`. Every answer cites its source.
Notation: `px4:` = `px4-autopilot/` clone; `a2:` = `sqe-a2/` repo.

## Build / run / locate
**Q1. How do you build and run ONE student test, e.g. HysteresisStructural?**
A: From the PX4 root after applying the patch: `cmake . -G Ninja -DCONFIG=px4_sitl_test
-B build/px4_sitl_test && cd build/px4_sitl_test && ninja unit-HysteresisStructural &&
./unit-HysteresisStructural` (expect 11/11). Single-test filter: `make tests
TESTFILTER=HysteresisStructural` or `./unit-HysteresisStructural
--gtest_filter=HysteresisStructural.TrueToFalseArmTakenWhenExpired`. Source:
`a2:submission_pkg/RUN_INSTRUCTIONS.md:21-30`, `a2:REPORT_DRAFT.md:109-113`.

**Q2. Same for the other two binaries?**
A: `ninja unit-PIDStructural && ./unit-PIDStructural` (17/17, GTest unit, Ashar-authored);
`ninja functional-BatteryStructural && ./functional-BatteryStructural` (33/33, GTest
functional). Upstream baselines: `./unit-Hysteresis` 7/7, `./unit-PID` 4/4.
Source: `a2:evidence/logs/student_*_run.log`, `a2:submission_pkg/RUN_INSTRUCTIONS.md:28-33`.

**Q3. Where is the H-D5 decision and what are its atomic conditions?**
A: `px4:src/lib/hysteresis/hysteresis.cpp:77` — `if (_state && !_requested_state)`.
c1 = `_state`, c2 = `!_requested_state`. Guarded by the L75 entry check
`if (_requested_state != _state)`. Source: `px4:src/lib/hysteresis/hysteresis.cpp:75-77`.

**Q4. Where is B-D4 and what are its 4 conditions?**
A: `px4:src/lib/battery/battery.cpp:130` —
`if (_connected && !_battery_initialized && _internal_resistance_initialized && _params.n_cells > 0)`.
c1 connected, c2 !initialized, c3 IR-init flag, c4 n_cells>0. Fires
`resetInternalResistanceEstimation`. Source: `px4:src/lib/battery/battery.cpp:130-132`.

**Q5. Where is P-D3 and its conditions?**
A: `px4:src/lib/pid/PID.cpp:70` — `if ((dt > FLT_EPSILON) && std::isfinite(_last_feedback))`.
c1 = `dt > FLT_EPSILON`, c2 = `isfinite(last)`. Guards the D-term.
Source: `px4:src/lib/pid/PID.cpp:66-74`.

## Independence pairs (one per MC/DC decision minimum)
**Q6. H-D5 independence pairs?**
A: c1 (`_state`): HST-02 (state T, req F, pre-expiry → armed/waiting) vs HST-03
(state F, req F → quiescent) — only c1 differs, both evaluate L77. c2: HST-01
(fires at expiry) vs HST-04 (req==state → L75 guard false, L77 not evaluated) — valid
end-to-end but NOT a strict same-decision flip; disclosed as guard-masked.
Source: `a2:MCDC_MATRIX.md:7-14`, `px4:src/lib/hysteresis/hysteresis.cpp:75-81`.

**Q7. H-D7 pairs?**
A: Mirror of H-D5 at `px4:src/lib/hysteresis/hysteresis.cpp:83` (`!_state && _requested_state`):
c1 via HST-06 (armed/waiting) vs HST-07 (quiescent); c2 via HST-05 (fires) vs HST-08
(req==state, L75 false). Same masking caveat as Q6.
Source: `a2:MCDC_MATRIX.md:19-26`.

**Q8. B-D4 pairs (all four)?**
A: Base BST-05 (T,T,T,T → ocv 16.0 fires). c1: BST-06 unconnected (F,T,T,T → 0.0).
c2: BST-07 mid (pre-init fires 15.0) vs final (post-init holds) — sequential calls in ONE
test, disclosed. c3: BST-08 IR-flag cleared (T,T,F,T → 0.0). c4: BST-09 n_cells 0
(T,T,T,F → 0.0). Observable: `ocv_estimate_filtered` with I=0.
Source: `a2:MCDC_MATRIX.md:30-38`, `a2:TEST_INVENTORY.md:29-33`.

**Q9. B-D8 pairs?**
A: `px4:src/lib/battery/battery.cpp:327-328`, c1 = `n_cells>0`, c2 = `V > n·Vch·1.05`.
Base BST-10 (T,T → SPIKES set, V 17.65). c1: BST-12 (n 0, V 100 → clean).
c2: BST-11 (exact 17.64 → clean, proves strict `>`). Source: `a2:MCDC_MATRIX.md:42-47`.

**Q10. B-D9 pairs and the gap?**
A: `px4:src/lib/battery/battery.cpp:370-371` (`!finite || <eps || fw_transition`).
c2: BST-18 call 1 (near-zero → reset 5.0) vs call 2 (all-false → holds).
c3: BST-20 call 1 (transition → reset, target moved 5.0→7.0) vs call 2 (consumed → hold).
c1 (`!finite`): NO test — unreachable via public API (sole writer L380/L383 guarded by
B-D10 finite check; `reset()` installs finite `BAT_AVRG_CURRENT`). Accepted investigated gap.
Source: `a2:MCDC_MATRIX.md:51-58`.

**Q11. B-D10 pairs? (c2 CLOSED 2026-10-02 — old "incomplete" wording retired)**
A: `px4:src/lib/battery/battery.cpp:375` (`armed && finite(current)`). c1: BST-18
(unarmed → holds 5.0) vs BST-19 (armed rotary → tracks, avg>5 asserted). c2: BST-19
(armed+T, finite+T → tracks) vs BST-31 (armed+T, finite+F via NaN current → skips
update, holds 5.0) — only c2 differs, outcome flips track→hold. `BRDA 375,0,3` now
taken (verified in final `.info`). Source: `a2:MCDC_MATRIX.md:60-68`,
`a2:evidence/final/scope_coverage_branch.info`.

**Q12. B-D11 pairs?**
A: `px4:src/lib/battery/battery.cpp:377-378` (`!is_fw || (fresh && LEVEL)`).
c1: BST-19 (multirotor → updates) vs BST-20 (FW stale → holds) — valid under masking
(c2/c3 don't-care when c1=T). c2: BST-20 (stale → hold) vs BST-27 (fresh LEVEL → update).
c3: BST-27 (LEVEL → update) vs BST-29 (CLIMB → hold). Source: `a2:MCDC_MATRIX.md:72-77`.

**Q13. P-D3 quad? (Ashar-authored file, lead-verified; D-term is PLUS per PID.cpp L47)**
A: PST-07 (F,F → 0) base; PST-08 (T,F → 0) isolates dt; PST-09 (T,T → +4.0 active)
isolates history; PST-10 (F,T → 0) re-confirms. Boundary PST-11 (`dt==eps` → 0) vs
PST-12 (`2eps` → +100 saturation). P-D2 single `isfinite` shown T (all integral tests)
+ F via PST-05 (NaN held, integral stays 0) and PST-06 (−Inf clamps +10, integral held).
Saturation/clamp pairs PST-02..04 assert accumulate-and-clamp (v1.17.0 has NO
conditional-integration freeze gate — L57-64 always integrates). Source: `a2:MCDC_MATRIX.md:79-89`.

## Levels / scope / short-circuit
**Q14. Why is hysteresis/PID unit and battery functional?**
A: Hysteresis/PID have internal-only deps (caller timestamps, floats) — deterministic without
framework support (`px4_add_unit_gtest`). Battery needs params (`BAT_*`), uORB
(`vehicle_status`, `flight_phase_estimation`) and time — requires `px4_add_functional_gtest`
with `gtest_functional_main`/px4_layer/uORB/params. SITL unnecessary (no full
flight-controller context needed). Source: `a2:REPORT_DRAFT.md:44-46,70`.

**Q15. Why is battery the MC/DC component?**
A: Its warning/fault classification drives failsafe (missed EMERGENCY risks loss of vehicle,
spurious aborts mission); it contains 5 non-trivial compounds (L130/B-D4, L327-328/B-D8,
L370-371/B-D9, L375/B-D10, L377-378/B-D11) governing that critical behaviour. Trivial
(SlewRate branch-free, circuit_breaker single-&&, logging/wrappers) excluded with reason.
Source: `a2:REPORT_DRAFT.md:46-48`, `a2:SCOPE_DECISIONS.md:30-49`.

**Q16. How does short-circuiting change MC/DC design?**
A: Masked conditions need no independent value when an earlier operand decides the outcome
(e.g. B-D11 c1=T masks freshness/LEVEL — BST-19 valid without fixing them). Conversely,
guards can make combos unreachable (L75 masks L77/L83 c2 sides). Pairs must account
for masking and flag unreachable combos with BRDA proof instead of forcing fake tests.
(Closed example: `||` at battery.cpp:61 once hid `index<1` when `index>9` — BST-30
now drives index 0 directly.) Source: `a2:MCDC_MATRIX.md:14,26`,
`px4:src/lib/hysteresis/hysteresis.cpp:75-83`.

## Coverage loss / gaps / baseline
**Q17. What coverage is lost if BST-11 (exact-boundary) is removed?**
A: B-D8 c2 independence collapses (only above-boundary BST-10 left; strict-`>` unproven) and
`BRDA:328,0,1` (false-side, count 50) loses its exact-boundary driver — the `>` vs `>=`
distinction becomes untested. Source: `.info` counts `328,0,0,2 / 328,0,1,50`.

**Q18. If a P-D2 test is removed?**
A: Removing PST-05/PST-06 loses the false side (`BRDA:61,0,1` → 0): non-finite integral
path untested and `PID.cpp:61-63` guard never takes F. Baseline PID was 8/10 for exactly
these missing sub-arcs. Source: baseline `.info` (BRH 8) vs final (BRH 10).

**Q19. If HST-01/HST-05 (expiry) are removed?**
A: `>=` true-sides at hysteresis.cpp:79/85 lose drivers (`BRDA 79,0,0,19` / `85,0,0,26`);
exact-expiry boundaries upstream never probes become untested. Lines stay covered via other
tests but decision outcomes collapse to one side.

**Q20. Battery line gap?**
A: L72 `PX4_ERR("Could not find parameter...")` (`DA:72,0`) — `param_find` never returns
`PARAM_INVALID` with intact metadata. Needs scratch/corrupted param store (fault injection).
Source: `px4:src/lib/battery/battery.cpp:68-73`, `.info` `DA:72,0`.

**Q21. Hysteresis 3-arc gap?**
A: `BRDA 77,0,3` + `83,0,1` + `83,0,3` = 0 — operand combos contradicting the L75 entry guard
(`requested != state`). Reachable only by removing the guard (forbidden). Accept as
provably unreachable. Source: `px4:src/lib/hysteresis/hysteresis.cpp:75-83`.

**Q22. B-D9 c1 gap in one sentence?**
A: Non-finite average unreachable because its only writer is guarded by B-D10's finite check
and reset installs a finite constant — defense-in-depth, concrete justification, not
"hardware dependent". Source: `px4:src/lib/battery/battery.cpp:370-383`.

**Q23. Baseline vs final contribution?**
A: Baseline `.info`: battery ABSENT (0%, no upstream test links it); hysteresis 22/22+17/20;
PID 22/22+8/10. Final: battery 230/231+174/248+19/19 fns; hysteresis same 22/22+17/20;
PID 22/22+10/10. Our suite contributed 0→99.6% lines on battery, closed PID's 2 sub-arcs,
and closed B-D10 c2 + L359 + L61-<1 sub-arcs with BST-30..32 probes.
Source: `a2:evidence/baseline/scope_baseline_branch.info`, `a2:evidence/final/scope_coverage_branch.info`.

**Q24. Testability/CMake changes and why behavior is unchanged?**
A: `+1` line in `src/lib/hysteresis/CMakeLists.txt` and `src/lib/pid/CMakeLists.txt`
(`px4_add_unit_gtest`), `+2` in `src/lib/battery/CMakeLists.txt`
(`px4_add_functional_gtest`); 3 new test files, 1140 insertions, 0 production-line edits
(`git diff` proof; `apply --check` clean). Registrations only add build targets; no
production symbol changed. Source: `a2:patch/full_reconstruction.patch`,
`a2:REPORT_DRAFT.md:72`.

**Q25. The early test failures — what happened?**
A: 8 failures total, all test-side expectation errors, production untouched: 2 Battery
RLS-gate tests (L130 model inverted — gate fires only pre-init + connected + IR-flag-set);
1 PID NaN fallback (`P*(-Inf)` is NaN, guard held); 5 Ashar PID (2 wrong D-signs —
v1.17.0 is PLUS; 3 assumed freeze gate that doesn't exist). Fixed expectations;
final suites 11/11 + 17/17 + 33/33. Source: `a2:REPORT_DRAFT.md:88`,
`a2:ASHAR_HANDOFF.md:171-180`.

**Q26. PID authorship — who wrote PIDStructuralTest.cpp?**
A: Ashar Ahmed (24i3072), from his own derivation (`ASHAR_HANDOFF.md`, branch
`ashar-pid-work`); lead-verified on Linux with 5 expectations corrected against v1.17.0
source (D-term PLUS sign L47; no freeze gate L57-64), final 17/17 PASS. His delivery is
preserved in `ashar/`; corrections documented in-test and in handoff §12.
Source: `a2:ASHAR_HANDOFF.md:171-224`, `a2:AI_ASSISTANCE_LOG.md`.

**Q27. What do BST-30..33 add?**
A: BST-30 (index 0) covers the `index<1` operand hidden by `||` short-circuit;
BST-31 (armed + NaN current) completes B-D10 c2 independence (`BRDA 375,0,3` now taken);
BST-32 (second FW copy) covers the L359 already-FW sub-arc; BST-33 (unadvertised topic)
documents safe behavior when `copy()` fails (L356-false investigated, not deterministically
reachable). Source: `a2:TEST_INVENTORY.md` BST-30..33 rows.
