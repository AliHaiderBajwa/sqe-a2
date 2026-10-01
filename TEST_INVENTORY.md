# Test inventory — working source for workbook Sheet 1 (Taimoor: convert to xlsx)
## Baseline: v1.17.0 @ d6f12ad. All results PASS (logs in evidence/logs/).

### Hysteresis — `unit-HysteresisStructural` (src/lib/hysteresis/HysteresisStructuralTest.cpp, GTest unit)

| Test ID | Component/function | Purpose / scenario | Key controlled input/state | Expected result | Exec | Structural target | Ref |
|---|---|---|---|---|---|---|---|
| HST-01 | update / T→F arm | fires exactly at expiry | req F@T0, win 3000, update T0+3000 | F | PASS | H-D5 T, H-D6 T(`>=`) | HysteresisStructural.TrueToFalseArmTakenWhenExpired |
| HST-02 | update / T→F pending | armed but waiting 1us before expiry | same, update T0+2999 | T (still) | PASS | H-D5 T, H-D6 F | ...ArmedButWaitingBeforeExpiry |
| HST-03 | update / quiescent F | no pending request stays F (c1 pair) | state F, no request | F | PASS | H-D4 F, H-D5 c1=F | ...QuiescentFalseShowsNoArm |
| HST-04 | set_state_and_update / no-op | request==state → no transition (c2 pair) | state T, req T | T | PASS | H-D2 F-path, H-D4 F | ...NoRequestMeansNoTransition |
| HST-05 | update / F→T arm | fires exactly at expiry | req T@T0, win 5000, update T0+5000 | T | PASS | H-D7 T, H-D8 T | ...FalseToTrueArmTakenWhenExpired |
| HST-06 | update / F→T pending | armed but waiting | same, update T0+4999 | F (still) | PASS | H-D7 T, H-D8 F | ...FalseToTrueArmedButWaitingBeforeExpiry |
| HST-07 | update / quiescent T | no pending stays T (c1 pair) | state T, no request | T | PASS | H-D4 F | ...QuiescentTrueShowsNoArm |
| HST-08 | set_state_and_update / no-op | request==state → no transition (c2 pair) | state F, req F | F | PASS | H-D2 F-path | ...NoRequestMeansNoTransitionUpwards |
| HST-09 | set_state_and_update / repeat | repeat request keeps original stamp | req T@T0, repeat T0+4000, win 5000 | F@4999, T@5000 | PASS | H-D3 F-path | ...RepeatRequestKeepsOriginalStamp |
| HST-10 | both directions | asymmetric windows independent | T→F win 3000, F→T win 5000 | F@T0+3000; F@T1+3000, T@T1+5000 | PASS | H-D5+H-D7 | ...AsymmetricWindowsIndependent |
| HST-11 | set_hysteresis_time_from | per-direction registers | from-true 100, from-false 7000 | F@T0+100, T@T0+7000 | PASS | H-D1 both | ...HysteresisTimesSetPerDirection |

### Battery — `functional-BatteryStructural` (src/lib/battery/BatteryStructuralTest.cpp, GTest functional)

Thresholds used: low 0.3, crit 0.15, emergen 0.07; n_cells 4, V_charged 4.2, V_empty 3.4 (unless stated).

| Test ID | Component/function | Purpose / scenario | Key controlled input/state | Expected result | Exec | Structural target | Ref |
|---|---|---|---|---|---|---|---|
| BST-01 | determineWarning | all four rungs | soc .5/.29/.14/.06/0 | NONE/LOW/CRIT/EMERG/EMERG | PASS | B-D7 all arms | WarningLadderRungs |
| BST-02 | determineWarning | strict-`<` exact-threshold boundary | soc .30/.15/.07 | NONE/LOW/CRIT | PASS | B-D7 boundaries | WarningExactThresholdTakesHigherRung |
| BST-03 | updateBatteryStatus | low voltage forces unconnected | V 1.5 (<2.1), connected | connected F | PASS | B-D1 T | LowVoltageForcesUnconnected |
| BST-04 | updateBatteryStatus | warning held until 2s init delay | ext soc .01, t T0 then T0+3s | NONE then EMERG | PASS | B-D3, B-D6 F→T | WarningHeldUntilInitialized |
| BST-05 | updateBatteryStatus L130 | RLS gate fires pre-init (base) | conn, pre-init, IR-flag T, n 4 | ocv 16.0 | PASS | B-D4 TTTT | RlsResetGateFiresPreInit |
| BST-06 | L130 | blocked when unconnected (c1) | V 1.0 | ocv 0.0 | PASS | B-D4 FTTT | ...BlockedWhenUnconnected |
| BST-07 | L130 | fires pre-init, holds post-init (c2) | V 16→15→14 @T0,+1s,+3s | 15.0 then holds 15.0 | PASS | B-D4 T/T→F on c2 | ...OnlyPreInit |
| BST-08 | L130 | blocked when IR flag cleared (c3) | n 4→0→4 param dance | ocv 0.0 | PASS | B-D4 TTFT | ...BlockedWhenResistanceInvalidated |
| BST-09 | L130 | blocked when n_cells 0 (c4) | n 0 | ocv 0.0, cells 0 | PASS | B-D4 TTTF | ...BlockedWhenNoCells |
| BST-10 | determineFaults | spike above 1.05 boundary | V 17.65 | SPIKES set | PASS | B-D8 TT | SpikeFaultAboveBoundary |
| BST-11 | determineFaults | exact boundary clean (strict `>`) | V 17.64 | SPIKES clear | PASS | B-D8 T,F | ...ExactlyAtBoundaryIsClean |
| BST-12 | determineFaults | needs n_cells>0 | n 0, V 100 | SPIKES clear | PASS | B-D8 F,x | ...NeedsCells |
| BST-13 | estimateStateOfCharge | fusion bounded + monotone | cap 1000, V 16.4, I 2, 8 iters | 0≤soc≤1, non-increasing | PASS | B-D12 T-arm | SocFusesAndDischargesMonotonically |
| BST-14 | updateDt/sumDischarged | 10s gap clamps dt to 2s | I 3.6, T0 then +10s | 2.0 mAh | PASS | L195-197, L204 | DtClampCapsDischargeIncrement |
| BST-15 | computeScale | scale inside [1,1.3] | V 16, init | 1≤scale≤1.3 | PASS | L340-341 | ScaleClampedToBand |
| BST-16 | computeScale | NaN fallback to 1.0 | V_charged 0, V 0 | 1.0 | PASS | L343-345 | ...FallsBackOnNonFinite |
| BST-17 | computeRemainingTime | no capacity → NaN | cap 0 | NaN | PASS | L389 F | RemainingTimeNeedsCapacity |
| BST-18 | computeRemainingTime | unarmed reset+hold | ext soc 1, cap 2000 | t finite>0; avg 5.0, holds 5.0 | PASS | B-D9 reset then hold; B-D10 F | ...UnarmedUsesAverage |
| BST-19 | computeRemainingTime | armed multirotor updates avg | ARMED rotary | t finite>0 | PASS | B-D10 T, B-D11 via !fw | ArmedMultirotorUpdatesAverage |
| BST-20 | computeRemainingTime | FW + stale phase holds avg | ARMED FW, phase ts=1 | avg 5.0, holds 5.0 (target 7) | PASS | B-D9 c3 pair; B-D11 F | ...StalePhaseHoldsAverage |
| BST-21 | updateTemperature | relay to status | 25.5 °C | 25.5 | PASS | L109-112 | TemperatureRelayedToStatus |
| BST-22 | publish paths | source match publishes | source 0==0 | no crash | PASS | L181-192 | UpdateAndPublishMatchesSource |
| BST-23 | publish paths | source mismatch skips | source param 99 | no crash | PASS | L183 F | ...SkipsOnSourceMismatch |
| BST-24 | ctor | index clamp + error path | index 99 | id 1 | PASS | L53, L61-63 | OutOfRangeIndexClampsToOne |
| BST-25 | resetInternalResistance | user R used at reset | r 0.01, I 0 | ocv 16.0 | PASS | L279-280 | UserInternalResistanceUsedInReset |
| BST-26 | calculateSoC | user R load correction | r 0.01, I 5, post-init | soc finite in [0,1] | PASS | L226-227 | ...CorrectsCellVoltage |
| BST-27 | computeRemainingTime | FW + fresh LEVEL updates avg | ARMED FW, fresh LEVEL | avg>5.0, t finite | PASS | B-D11 TTT | ...FreshLevelPhaseUpdatesAverage |
| BST-28 | computeRemainingTime | FW + zero dt else-path | single update (_dt 0) | avg finite | PASS | L379 F→L383 | ...ZeroDtUpdatesAverageWithoutDt |
| BST-29 | computeRemainingTime | FW + fresh CLIMB holds | ARMED FW, fresh CLIMB | avg 5.0 | PASS | B-D11 c3=F | ...NonLevelPhaseHoldsAverage |

### PID — `unit-PIDStructural` (src/lib/pid/PIDStructuralTest.cpp, GTest unit)

| Test ID | Component/function | Purpose / scenario | Key controlled input/state | Expected result | Exec | Structural target | Ref |
|---|---|---|---|---|---|---|---|
| PST-01 | update + updateIntegral | integral frozen when flag cleared, then evolves | P2/I5, sp1, fb0, dt0.1, flag F then T | 2.0 + int 0; then 2.0 + int 0.5 | PASS | P-D1 F→T | IntegralFrozenWhenUpdateFlagCleared |
| PST-02 | output clamp + integral | high-side saturation clamps output, integral accumulates (no freeze gate in v1.17.0) | I.5/lim10/out±.2, sp2, fb0, dt.5 ×2 | 0 + int.5; then .2 + int 1.0 | PASS | saturation edges | SaturationHighClampsOutputIntegralNotFrozen |
| PST-03 | output clamp + integral | low-side mirror | sp−2 mirror | 0 + int−.5; then −.2 + int −1.0 | PASS | saturation edges | SaturationLowClampsOutputIntegralNotFrozen |
| PST-04 | integral unwind | error reversal under saturation unwinds integral | saturated-high, then sp→−2 | .2 + int 0.5 | PASS | saturation edges | SaturationHighThenReversalUnwindsIntegral |
| PST-05 | updateIntegral | NaN feedback discards integral candidate | P1/I.1, sp1, fb NaN | NaN out + int 0; then 0 + int 0 | PASS | P-D2 F-path | NonFiniteIntegralCandidateIsDiscarded_NaN |
| PST-06 | updateIntegral + output clamp | −Inf feedback clamps output, integral held | P1/I.1, sp1, fb −Inf | +10 + int 0; then 0 + int 0 | PASS | P-D2 F-path + clamp | ..._NegInfSaturates |
| PST-07 | updateDerivative | guard all-false (dt 0, last NaN) | D4, fb2.0, dt0, fresh | 0 | PASS | P-D3 FF | DerivativeGuardBlocksWhenNoTimeAndNoHistory |
| PST-08 | updateDerivative | time ok, no history (dt .5, last NaN) | D4, fb2.0, dt.5, fresh | 0 | PASS | P-D3 TF | ...TimeOkButNoHistory |
| PST-09 | updateDerivative | fully active (PLUS sign, L47) | D4, seed 2.0/.5, then 2.5/.5 | +4.0 | PASS | P-D3 TT | ...ActiveWhenTimeAndHistory |
| PST-10 | updateDerivative | history present but dt 0 | D4, seed 2.0/.5, then 3.0/dt0 | 0 | PASS | P-D3 FT | ...HistoryButNoTime |
| PST-11 | updateDerivative | exact dt==EPS boundary (not >) | D4, seed 2.0/.5, then 2.5/dt=EPS | 0 | PASS | P-D3 boundary | ...ExactEpsilonIsNotEnoughTime |
| PST-12 | updateDerivative | just above EPS saturates high via D | D4, 2.5/dt=2EPS, lim±100 | +100 (deriv 2097152 exact) | PASS | P-D3 T + saturation | ...JustAboveEpsilonSaturatesHigh |
| PST-13 | updateDerivative | negative dt robustness | P2/D4, sp1, fb.5, dt−.5 | 1.0 (P-only) | PASS | P-D3 invalid input | NegativeDtDisablesDerivativeTerm |
| PST-14 | resetDerivative | history cleared to NaN, guard re-entry | D2, seed 0/.5, reset, then 1.0/.5 | 0 | PASS | P-D3 TF re-entry | ResetDerivativeClearsHistory |
| PST-15 | updateIntegral | windup clamps exactly at limit | I1/lim.5/out±10, sp1, fb0, dt.5 ×3 | 0, .5, .5; int .5 | PASS | P-D1 T + clamp | IntegralWindupClampsExactlyAtLimit |
| PST-16 | output limit | exact-boundary passthrough + clamp | P1/lim±2, sp5, fb 3/2/4 | 2.0 / 2.0 / 1.0 | PASS | saturation edges | OutputLimitPassesExactBoundaryThrough |
| PST-17 | degenerate config | zero gains hold zero | gains 0, out±5, sp3, fb1 | 0 + int 0 | PASS | degenerate | ZeroGainDegenerateConfigHoldsZero |

> Owner: Ashar Ahmed (24i3072). Authored by Ashar from his own derivation (ASHAR_HANDOFF.md); lead-verified on Linux — 5 expectations corrected against v1.17.0 source during verification (D-term PLUS sign L47: 2 tests; no conditional-integration freeze gate L57-64: 3 tests), final 17/17 PASS. Replaces the earlier 11-test lead fallback; PST prefix kept for workbook stability.
