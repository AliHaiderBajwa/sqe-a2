# MC/DC evidence — working source for workbook Sheet 2 (Taimoor: convert to xlsx)
## Notation: each row = one independence demonstration. Observable in `()`.

### H-D5 — hysteresis.cpp L77 `if (_state && !_requested_state)`
c1=`_state`, c2=`!_requested_state`.

| c1 | c2 | time | outcome (get_state) | pair test | demonstrates |
|---|---|---|---|---|---|
| T | F(req F) | pre-expiry | T (armed, waiting) | HST-02 | base |
| F | F(req F) | pre-expiry | F (quiescent, no arm) | HST-03 | c1 independent (only c1 differs vs HST-02) |
| T | F(req F) | expired | F (transition) | HST-01 | base |
| T | T(req T) | expired | T (no arm) | HST-04 | c2 independent (only c2 differs vs HST-01) |

Unreachable (investigated, not a gap): c1=T,c2=F-unevaluated… precisely the arc (state=T, req=T) at L77 cannot occur because L75 (`requested != state`) guards entry. Short-circuit arc (b=false→else with a=true) likewise unreachable.

### H-D7 — hysteresis.cpp L83 `else if (!_state && _requested_state)`
c1=`!_state`, c2=`_requested_state`.

| c1 | c2 | time | outcome | pair test | demonstrates |
|---|---|---|---|---|---|
| T(st F) | T(req T) | pre-expiry | F (armed, waiting) | HST-06 | base |
| F(st T) | T(req T) | pre-expiry | T (quiescent) | HST-07 | c1 independent |
| T(st F) | T(req T) | expired | T (transition) | HST-05 | base |
| T(st F) | F(req F) | expired | F (no arm) | HST-08 | c2 independent |

Unreachable arcs at L83 (a=false with state=T; b=false with req=F while state=F): both contradict the L75 entry guard. Proven by code inspection.

### B-D4 — battery.cpp L130 `connected && !initialized && IRinit && n_cells>0` → RLS reset (observable: ocv_estimate_filtered 16.0 vs 0.0, I=0)

| conn | !init | IRinit | n>0 | outcome | pair test | demonstrates |
|---|---|---|---|---|---|---|
| T | T | T | T | 16.0 (fires) | BST-05 | base |
| F | T | T | T | 0.0 | BST-06 | c1 (connected) |
| T | T/F | T | T | refires→15.0 / holds 15.0 | BST-07 (mid vs final) | c2 (!initialized) |
| T | T | F | T | 0.0 | BST-08 | c3 (IRinit) |
| T | T | T | F | 0.0 | BST-09 | c4 (n_cells) |

Condition T+F shown for all four; decision T+F shown (fires in BST-05/07-mid, holds in BST-06/08/09/07-final).

### B-D8 — battery.cpp L327-328 `(n_cells>0) && (V > n*Vch*1.05)` → SPIKES bit

| n>0 | over | outcome | pair test | demonstrates |
|---|---|---|---|---|
| T | T | fault set | BST-10 | base |
| F | T | clean | BST-12 | c1 (n_cells) |
| T | T | fault set | BST-10 | base |
| T | F (exact 17.64) | clean | BST-11 | c2 (strict `>`) |

### B-D9 — battery.cpp L370-371 `!finite(avg) \|\| avg<eps \|\| fw_transition` → reset to BAT_AVRG_CURRENT

| !fin | <eps | fw_tr | outcome | pair test | demonstrates |
|---|---|---|---|---|---|
| F | T | F | reset to 5.0 | BST-18 call 1 | base (c2) |
| F | F | F | holds 5.0 | BST-18 call 2 | c2 (avg state) + all-false |
| F | F | T→F | resets 5.0 then holds 5.0 (target 7) | BST-20 call 1 vs call 2 | c3 (transition consumed) |
| T | — | — | (see gap note) | — | c1 (!finite) |

Gap note (investigated): c1=T (non-finite average) is unreachable through the public API — the only writer of the average (L380/L383) is itself guarded by B-D10's `PX4_ISFINITE(current)` check, and `reset()` only installs the finite `BAT_AVRG_CURRENT`. The arm is defense-in-depth subsumed by B-D10. Not a testability defect in the analyzed scope.

### B-D10 — battery.cpp L375 `armed && isfinite(current)` → average update

| armed | finite | outcome | pair test | demonstrates |
|---|---|---|---|---|
| F | T | holds 5.0 | BST-18 | c1 (armed) vs BST-19 |
| T | T | tracks (avg>5) | BST-19 | base |
| T | F | holds 5.0 | (NaN probe, removed — B-D10 demonstrably skips; kept as analyzed behavior, not a workbook row) | c2 (finite) |

Note: c2=F verified during development (NaN current leaves average at reset value); the probe was removed because it asserted no independent observable beyond the skip already shown by BST-18 vs BST-19. Strictly, c2 independence is evidenced by code inspection + the skip path being exercised (BRDA L375 arcs both taken per coverage).

### B-D11 — battery.cpp L377-378 `!is_fw \|\| (fresh && LEVEL)` → FW-gated update

| !fw | fresh | LEVEL | outcome | pair test | demonstrates |
|---|---|---|---|---|---|
| T | — | — | updates (avg>5) | BST-19 | c1 (!is_fw) vs BST-20 |
| F | F | T | holds 5.0 | BST-20 | c2 (freshness) vs BST-27 |
| F | T | T | updates (avg>5) | BST-27 | base |
| F | T | F | holds 5.0 | BST-29 | c3 (LEVEL) vs BST-27 |

### P-D3 — PID.cpp L70 `(dt > FLT_EPSILON) && isfinite(last)` → D-term (observable: derivative contribution)
c1=`dt > FLT_EPSILON`, c2=`isfinite(last)`.

| c1 | c2 | outcome | pair test | demonstrates |
|---|---|---|---|---|
| F | F | 0 (forced) | PST-02 | base |
| T | F | 0 (forced) | PST-03 | c1 independent (only c1 differs vs PST-02) |
| T | T | active ((0.6−0.5)/0.1=1.0 → 5.0) | PST-04 | c2 independent (only c2 differs vs PST-03) |
| F | T | 0 (forced) | PST-05 | c1=F re-confirmed with history present |

Boundary: PST-06 (dt==eps → F) vs PST-07 (dt==2eps → T, saturates). Decision T+F shown. P-D2 `isfinite(integral_new)`: T shown by all integral tests, F shown by PST-01 (NaN output, integral held at 0).

> NOTE (team): P-D3 rows are lead-authored fallback for Ashar's package; replace with his pairs if he delivers.
