# Decision inventory (working file → feeds workbook Sheet 1/2 + viva bank)
## Scope: hysteresis.cpp + PID.cpp + battery.cpp @ d6f12ad (v1.17.0)

### H1. `src/lib/hysteresis/hysteresis.cpp` (GTest unit)

| ID | Location | Decision | Type | Obligation |
|---|---|---|---|---|
| H-D1 | L48 `if (from_state)` | T/F | branch | set true-time vs false-time |
| H-D2 | L59 `if (new_state != _state)` | T/F | branch | state-change request vs no-change |
| H-D3 | L60 `if (new_state != _requested_state)` | T/F | branch | fresh request (stamp time) vs repeat request (keep stamp) |
| H-D4 | L75 `if (_requested_state != _state)` | T/F | branch | pending transition vs settled |
| H-D5 | L77 `if (_state && !_requested_state)` | T/F + MC/DC c1,c2 | compound `&&` | true→false direction guard |
| H-D6 | L79 `if (now >= last + time_true)` | T/F | branch + boundary | exact-equality flips (>=) |
| H-D7 | L83 `else if (!_state && _requested_state)` | T/F + MC/DC c1,c2 | compound `&&` | false→true direction guard |
| H-D8 | L85 `if (now >= last + time_false)` | T/F | branch + boundary | exact-equality flips (>=) |

MC/DC pairs: H-D5 — (T,T)→F vs (F,T)→T isolates c1 `_state`; (T,T)→F vs (T,F)→T isolates c2 `!_requested_state`. H-D7 symmetric.
Boundary focus: `>=` at exact expiry instant; zero hysteresis time (immediate flip); re-request same value keeps original stamp; `update()` with nothing pending is a no-op.

### P1. `src/lib/pid/PID.cpp` (GTest unit, owner: Ashar)

| ID | Location | Decision | Type | Obligation |
|---|---|---|---|---|
| P-D1 | L49 `if (update_integral)` | T/F | branch | integral state evolves vs frozen |
| P-D2 | L61 `if (isfinite(integral_new))` | T/F | branch | NaN/Inf input must NOT corrupt `_integral` |
| P-D3 | L70 `if ((dt > FLT_EPSILON) && isfinite(_last_feedback))` | T/F + MC/DC | compound `&&` | D-term active only when both hold |

Gap notes (upstream `PIDTest.cpp` misses): non-finite integral path (Inf/NaN feedback), `dt == FLT_EPSILON` boundary, `resetDerivative()`→NaN-last with D gain set, negative dt, exact-limit saturation both sides.

### B1. `src/lib/battery/battery.cpp` (GTest functional, MC/DC component)

Critical behaviour: warning/fault classification driving failsafe.

| ID | Location | Decision | Conditions | Note |
|---|---|---|---|---|
| B-D1 | L119 `voltage < RECOGNITION` | single | c1 | unconnected override |
| B-D2 | L123 `!connected \|\| last_unconn==0` | `\|\|` | c1,c2 | timestamp latch |
| B-D3 | L128 init gate | `&&` (2) | connected, time-passed | filter-init delay |
| B-D4 | L130 RLS-reset gate | `&&` (4) | connected, !initialized, IR-init, n_cells>0 | richest MC/DC target |
| B-D5 | L138 `!external_soc` | single | — | fusion on/off |
| B-D6 | L144 `connected && initialized` | `&&` (2) | — | warning arming |
| B-D7 | L309–319 warning ladder | 3×`<` chain | emerg/crit/low/none | boundary at each threshold ±ε |
| B-D8 | L327–328 spike fault | `&&` (2) | n_cells>0, V > n·Vch·1.05 | over-voltage, 1.05 boundary |
| B-D9 | L370–371 avg-filter reset | `\|\|` (3) | !finite, <eps, FW-transition | MC/DC triple |
| B-D10 | L375 armed-update gate | `&&` (2) | armed, finite current | — |
| B-D11 | L377–378 FW level-flight gate | mix (3) | !is_fw, recent stamp, LEVEL phase | short-circuit sensitive |
| B-D12 | L215/223/226/253/279 SoC/IR branches | singles | — | n_cells==0, current>eps, r_internal>=0, covariance-improves |

Excluded from MC/DC (with reason): L183 publish-source check (trivial routing), L196/L204/L389 guards (single-condition numerics), L400/L415/L419 param-init (test-harness setup, not critical behaviour).
