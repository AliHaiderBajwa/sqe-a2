# TASK 2 — Coverage Gap Analysis (Taimoor)
SUT: PX4 v1.17.0 @ `d6f12ad1c4f70ad3230afd7d86e971421e02fef4`.
Evidence: `sqe-a2/evidence/final/scope_coverage_branch.info` (+ `evidence/final/html/`),
`evidence/baseline/scope_baseline_branch.info`, source in `px4-autopilot/src/lib/`.
Scope files only: `battery/battery.cpp`, `hysteresis/hysteresis.cpp`, `pid/PID.cpp`.

## Summary counts (from `.info`)
- `battery.cpp`: 230/231 lines (gap: L72), 19/19 fns, 170/248 branch arcs (78 zero-hit).
- `hysteresis.cpp`: 22/22 lines, 17/20 arcs (3 zero-hit: `BRDA 77,0,3` + `83,0,1` + `83,0,3`).
- `PID.cpp`: 22/22 lines, 10/10 arcs — NO gaps.

## Key correction to current prose
`REPORT_DRAFT.md §7` reports the totals correctly but explains only L72. Of the 78 battery
zero-hit arcs, ~70 are `e0` exception/unwind arcs from C++ matrix/filter/param machinery
(class ii, not source decisions). The source-relevant zeros are exactly FIVE:
`71,0,0` (L72 guard true-side), `371,0,1` + `371,0,2` (B-D9 c1), `375,0,3` (B-D10 c2),
plus short-circuit sub-arcs at `54,0,1` / `61,0,2` (ctor index clamp) and
`356,0,3` / `359,0,3` (vehicle_status copy/FW-transition false paths). Everything else is class (ii).

## Gap table (file | line | arc | class | justification or needed test)
Classes: (i) real untested decision outcome · (ii) compiler-generated exception/template arc ·
(iii) provably unreachable/defensive · (iv) ternary/short-circuit artifact.

| file | line | arc (BRDA) | class | justification / needed test |
|---|---|---|---|---|
| battery.cpp | 72 | `72,0,0,-` + `72,e0,1,-` (line DA 72 hit 0) | (iii) | `PX4_ERR("Could not find parameter ...")` fires only if `param_find("BAT%d_V_EMPTY")` returns `PARAM_INVALID` (guard L71). With intact v1.17.0 param metadata every `BAT*_V_EMPTY` exists, so the true-side is unreachable locally. Needed to reach: corrupted/scratch param store or `param_find` stub returning `PARAM_INVALID` (fault injection). Pass to Ali as a param-stub test; do NOT hand-wave as "hardware dependent". |
| battery.cpp | 71 | `71,0,0,0` | (iii) | True-side of the L71 `== PARAM_INVALID` check — same defensive path as L72; never taken because lookup always succeeds. Same fault-injection requirement. |
| battery.cpp | 371 | `371,0,1,0` + `371,0,2,0` (B-D9 c1) | (iii) | `!PX4_ISFINITE(avg)` never T via public API: sole average writer L380/L383 is itself guarded by B-D10's `PX4_ISFINITE(current)` (battery.cpp:375), and `reset()` (battery.cpp:372) installs finite `BAT_AVRG_CURRENT`. Defense-in-depth subsumed downstream. Investigated gap, technically concrete — keep justification, accept incomplete c1. |
| battery.cpp | 375 | `375,0,3,0` (B-D10 c2, armed=T + finite=F) | (i) REAL GAP | No submitted test drives `armed=T` with non-finite current. The NaN probe was removed (MCDC_MATRIX.md:66). Needed test for Ali: arm (publish `ARMING_STATE_ARMED` via fixture-held `vehicle_status`), then `computeRemainingTime(NAN)`, assert average holds reset value. Do NOT claim c2 demonstrated until this test exists + re-measure. |
| battery.cpp | 356 | `356,0,3,0` | (i) REAL GAP | `_vehicle_status_sub.copy()` false-path (copy fails / no publication) never taken — fixture always holds pubs so `copy()` succeeds. Needed test: fresh Battery with NO vehicle_status publication, call `getBatteryStatus()`/`computeRemainingTime`, assert `_armed` unchanged. UORB-failure injection. |
| battery.cpp | 359 | `359,0,3,0` | (i) REAL GAP (minor) | FW-transition sub-condition outcome never taken in one polarity: tests cover MR (BST-19), FW-stale (BST-20), FW-fresh-LEVEL (BST-27), FW-fresh-CLIMB (BST-29), but the `vehicle_type==FW && !_is_fw` reset arm's false-with-true Mixer path at 359 is not hit as a distinct sub-arc. Needed test: publish FW type twice in a row (second copy with `_is_fw` already true → reset flag false) and assert no spurious reset. Low risk; document. |
| battery.cpp | 53-54 | `54,0,1,0` | (iv) | Ternary `index<1\|\|index>9 ? 1 : index` (battery.cpp:53) — one sub-arc zero is the unclamped/clamped partition artifact of the ternary + `||` short-circuit. Both behaviors ARE tested (BST-24 index 99 clamps; all others pass through). No new test; note as artifact. |
| battery.cpp | 61 | `61,0,2,0` | (iv)/(i) | `if (index>9\|\|index<1)` (battery.cpp:61): `index>9` T covered (BST-24, 99); `index<1` T NEVER covered (no negative/zero index test). Short-circuit hides it (99 short-circuits before `<1`). Recommended test for Ali: `Battery(0 or -1, ...)` assert clamp to 1 + error log. One-line addition, real input partition. |
| hysteresis.cpp | 77 | `77,0,3,0` | (iii) | `(state=T, req=T)` at L77 `if (_state && !_requested_state)` contradicts L75 entry guard `(_requested_state != _state)` (hysteresis.cpp:75). To evaluate L77 with c1=T,c2=F-unevaluated requires reaching L77 while L75 is false — impossible by construction. Guarding lines cited: 75 → 77. Would need guard removal (forbidden prod change) — accept as unreachable. |
| hysteresis.cpp | 83 | `83,0,1,0` + `83,0,3,0` | (iii) | Same L75 guard argument at L83 `else if (!_state && _requested_state)` (hysteresis.cpp:83): `state=T` (arc 1) or `req=F with state=F` (arc 3) while inside the `requested!=state` block is contradictory. Guarding lines: 75 → 83. Accept as unreachable; same forbidden-change argument. |
| battery.cpp | ctor/filters/matrix/params (70 arcs) | all `*,e0,*,0` listed in appendix | (ii) | Exception/unwind arcs from `AlphaFilter::setParameters`, `snprintf`/`param_find`, `Vector2f/Matrix2f` RLS math (battery.cpp:239-264), `powf/sqrtf`, `param_get`/`ModuleParams` (L400-431), `update()`/`publish` uORB paths. NOT source decisions; no test can target them from the API. Lcov handling + report wording below. |

## Appendix — full zero-hit BRDA list (battery.cpp, 78 entries)
`54,0,1` · `54,e0,3` · `54,e0,5` · `57,e0,1` · `58,e0,1` · `59,e0,1` · `61,0,2` · `62,e0,1` ·
`69,e0,1` · `71,0,0` · `72,0,0` · `72,e0,1` · `76,e0,1` · `79,e0,1` · `82,e0,1` · `85,e0,1` ·
`88,e0,1` · `90,e0,1` · `91,e0,1` · `92,e0,1` · `94,e0,1` · `96,e0,1` · `158,e0,1` · `167,e0,1` ·
`191,e0,1` · `191,e0,3` · `224,e0,1` · `235,e0,1` · `241,e0,1` · `242,e0,1` · `242,e0,3` ·
`244,e0,1/.3/.5/.7/.9/.11/.13` · `245,e0,1/.3/.5/.7` · `247,e0,1/.3/.5/.7/.9/.11/.13/.15` ·
`254,e0,1` · `255,e0,1` · `258,e0,1` · `261,e0,1` · `261,e0,3` · `264,e0,1` · `356,e0,1` ·
`356,0,3` · `359,0,3` · `371,0,1` · `371,0,2` · `375,0,3` · `380,e0,1` · `383,e0,1` ·
`401,e0,1` · `402,e0,1` · `403,e0,1` · `404,e0,1` · `405,e0,1` · `406,e0,1` · `407,e0,1` ·
`408,e0,1` · `409,e0,1` · `412,e0,1` · `413,e0,1` · `420,e0,1` · `421,e0,1` · `431,e0,1`.
Hysteresis zeros: `77,0,3` · `83,0,1` · `83,0,3`. PID zeros: none. (All `e0` = class ii.)

## Lcov exclusion guidance (for Ali / report §7)
- The class-(ii) `e0` arcs are gcov exception-handling arcs, not `if` outcomes in the source.
  They may be legitimately set aside ONLY as a reporting clarification, never as an excuse to
  drop difficult logic. No class-(i) arc (356/359/375, L72/71) and no B-D9/B-D10 decision arc
  may be excluded.
- Proposed report wording (paste into §7): "Branch totals include compiler-generated
  exception/unwind arcs (`e0` in `.info`, ~70 in battery.cpp from matrix/filter/param templates)
  that have no corresponding source decision and cannot be driven from the public API; all
  reachable source-decision outcomes are reported separately. The sole uncovered source line
  (L72) and the source-relevant uncovered sub-arcs (L71, L356, L359, L371×2, L375) are
  individually justified in the gap table; exclusions were applied to compiler-generated arcs
  only." If the team filters them mechanically, use an `--exclude`/`--filter` invocation that
  names `e0`/exception arcs explicitly, keep raw + filtered `.info` files, and state both counts.
- UNVERIFIED: whether the grading toolchain accepts filtered counts — keep the raw
  `scope_coverage_branch.info` (170/248) as the authoritative evidence and present any filtered
  view as supplementary, not as replacement.
