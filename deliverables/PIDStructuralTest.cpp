/****************************************************************************
 *
 *   Copyright (c) 2026 PX4 Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name PX4 nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file PIDStructuralTest.cpp
 *
 * Student-authored structural tests for PID (src/lib/pid/PID.cpp @ v1.17.0),
 * GTest unit level. Owner: Ashar Ahmed (24i3072).
 *
 * Derived from my own decision inventory of PID.cpp @ v1.17.0
 * (see ASHAR_HANDOFF.md), corrected during lead verification against source:
 *  - A-D1  L49 `if (update_integral)` — integral state evolves vs frozen.
 *  - A-D2  saturation/clamp edges (boundary obligations, NOT a compound
 *          decision: v1.17.0 `updateIntegral`, L57-64, has no
 *          conditional-integration freeze gate — it always integrates and
 *          only clamps to `_limit_integral`, guarding solely against
 *          non-finite candidates).
 *  - A-D3  L61 `if (isfinite(integral_new))` — non-finite candidate discarded.
 *  - A-D4  L70 `(dt > FLT_EPSILON) && isfinite(_last_feedback)` — D-term guard
 *          (MC/DC-style quad over dt x history, plus exact-EPS boundary).
 *  - Saturation edges of `math::constrain` on output and integral limits.
 *
 * Every expected value below is hand-computed from PID.cpp / PID.hpp source
 * (see derivation comments per test). Key source facts used:
 *  - `update()` (L47) uses the PRE-update integral for the returned output;
 *    the integral is updated afterwards via `updateIntegral()`.
 *  - D-term sign is PLUS: output = P*err + I + D*(feedback-last)/dt, so a
 *    RISING feedback produces a POSITIVE D contribution (verified by
 *    execution: +4.0 for the 2.0->2.5/0.5 step with D=4).
 *  - `constrain(NaN, lo, hi)` returns NaN (both comparisons false).
 *  - Exact-binary values (multiples of 0.5, dt = 2*FLT_EPSILON with a 0.5
 *    step) are used so expectations are bit-exact, not luck.
 *
 * Upstream PIDTest.cpp was read for conventions only. Gaps it leaves open
 * and this file closes: NaN / -Inf feedback guarding (A-D3 false path),
 * dt == FLT_EPSILON boundary, negative dt, resetDerivative() re-entry,
 * exact-limit saturation on both output and integral, zero-gain degenerate
 * config, and explicit saturation/clamp pairs with Ashar's own gain/limit
 * numbers (upstream uses .1/1/.05; mine use .5/10/.2).
 */

#include <gtest/gtest.h>

#include <cfloat>
#include <cmath>
#include <PID.hpp>

// A-D1: `update_integral == false` freezes the integral state even with error.
TEST(PIDStructural, IntegralFrozenWhenUpdateFlagCleared)
{
	// P=2, I=5, out +/-100, int lim 100, sp 1, fb 0 -> error 1.
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setIntegralLimit(100.f);
	pid.setGains(2.f, 5.f, 0.f);
	pid.setSetpoint(1.f);

	// Flag cleared: output = 2*1 + 0 = 2, integral untouched.
	EXPECT_FLOAT_EQ(pid.update(0.f, 0.1f, false), 2.f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.f);

	// Flag set: output still uses pre-update integral (2), then
	// integral = 0 + 5*1*0.1 = 0.5.
	EXPECT_FLOAT_EQ(pid.update(0.f, 0.1f, true), 2.f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.5f);
}

// Saturation, high side: v1.17.0 PID.cpp has NO conditional-integration
// freeze gate (updateIntegral, L57-64, only guards non-finite candidates).
// I=0.5, int lim 10, out +/-0.2, sp 2, fb 0, dt 0.5:
// Step 1: output = 0 (integral still 0), integral = 0 + 0.5*2*0.5 = 0.5.
// Step 2: output = 0.5 -> constrained to 0.2; integral KEEPS accumulating
// to 1.0 (clamped only by the +-10 integral limit).
TEST(PIDStructural, SaturationHighClampsOutputIntegralNotFrozen)
{
	PID pid;
	pid.setOutputLimit(0.2f);
	pid.setIntegralLimit(10.f);
	pid.setGains(0.f, 0.5f, 0.f);
	pid.setSetpoint(2.f);

	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.5f);

	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.2f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), 1.f);
}

// Saturation, low side (mirror): output clamps to -0.2 while the integral
// accumulates to -1.0 — same no-freeze semantics as the high side.
TEST(PIDStructural, SaturationLowClampsOutputIntegralNotFrozen)
{
	PID pid;
	pid.setOutputLimit(0.2f);
	pid.setIntegralLimit(10.f);
	pid.setGains(0.f, 0.5f, 0.f);
	pid.setSetpoint(-2.f);

	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), -0.5f);

	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), -0.2f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), -1.f);
}

// Error reversal under saturation: after two saturated-high steps the
// integral stands at 1.0 (see above); reversing the setpoint to -2 gives
// error -2, output uses the pre-update integral 1.0 -> constrained 0.2,
// and the integral unwinds to 1.0 + 0.5*(-2)*0.5 = 0.5.
TEST(PIDStructural, SaturationHighThenReversalUnwindsIntegral)
{
	PID pid;
	pid.setOutputLimit(0.2f);
	pid.setIntegralLimit(10.f);
	pid.setGains(0.f, 0.5f, 0.f);
	pid.setSetpoint(2.f);

	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.f);   // integral 0.5
	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.2f);  // integral 1.0

	pid.setSetpoint(-2.f);
	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.2f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.5f);
}

// A-D3: NaN feedback -> integral candidate NaN -> guard holds integral at 0.
// P=1 so error path is exercised; constrain(NaN) passes NaN through.
TEST(PIDStructural, NonFiniteIntegralCandidateIsDiscarded_NaN)
{
	PID pid;
	pid.setOutputLimit(10.f);
	pid.setIntegralLimit(5.f);
	pid.setGains(1.f, 0.1f, 0.f);
	pid.setSetpoint(1.f);

	// error = 1 - NaN = NaN; output = P*NaN = NaN (passes through constrain);
	// integral_new = 0 + 0.1*NaN*0.1 = NaN -> not finite -> held at 0.
	EXPECT_TRUE(std::isnan(pid.update(NAN, 0.1f, true)));
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.f);

	// Next normal sample resumes cleanly: error 0 -> output 0, integral 0.
	EXPECT_FLOAT_EQ(pid.update(1.f, 0.1f, true), 0.f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.f);
}

// A-D3: -Inf feedback with P active -> +Inf error -> output clamps to
// +10, while the finiteness guard holds the integral (integral_new =
// 0 + 0.1*(+Inf)*0.1 = +Inf -> not finite -> discarded, stays 0).
TEST(PIDStructural, NonFiniteIntegralCandidateIsDiscarded_NegInfSaturates)
{
	PID pid;
	pid.setOutputLimit(10.f);
	pid.setIntegralLimit(5.f);
	pid.setGains(1.f, 0.1f, 0.f);
	pid.setSetpoint(1.f);

	// error = 1 - (-Inf) = +Inf; output +Inf -> constrained +10.
	// integral_new = +Inf -> not finite -> held at 0.
	EXPECT_FLOAT_EQ(pid.update(-INFINITY, 0.1f, true), 10.f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.f);

	EXPECT_FLOAT_EQ(pid.update(1.f, 0.1f, true), 0.f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.f);
}

// A-D4 quad (a=F, b=F): fresh object (last = NaN) with dt = 0 -> D forced 0.
TEST(PIDStructural, DerivativeGuardBlocksWhenNoTimeAndNoHistory)
{
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 4.f);
	pid.setSetpoint(0.f);
	EXPECT_FLOAT_EQ(pid.update(2.f, 0.f, false), 0.f);
}

// A-D4 quad (a=T, b=F): dt valid but no history (last still NaN) -> 0.
TEST(PIDStructural, DerivativeGuardBlocksWhenTimeOkButNoHistory)
{
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 4.f);
	pid.setSetpoint(0.f);
	EXPECT_FLOAT_EQ(pid.update(2.f, 0.5f, false), 0.f);
}

// A-D4 quad (a=T, b=T): fully active. Seed 2.0, step to 2.5 over dt 0.5:
// derivative = (2.5 - 2.0)/0.5 = 1.0 EXACT (binary-exact operands), and per
// PID.cpp L47 (output = P*e + I + D*deriv, PLUS sign) output = +4*1.0 = +4.0.
TEST(PIDStructural, DerivativeGuardActiveWhenTimeAndHistory)
{
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 4.f);
	pid.setSetpoint(0.f);

	EXPECT_FLOAT_EQ(pid.update(2.f, 0.5f, false), 0.f);   // seeds last = 2.0
	EXPECT_FLOAT_EQ(pid.update(2.5f, 0.5f, false), 4.f); // D active
}

// A-D4 quad (a=F, b=T): history present but dt = 0 -> D forced 0.
TEST(PIDStructural, DerivativeGuardBlocksWhenHistoryButNoTime)
{
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 4.f);
	pid.setSetpoint(0.f);

	EXPECT_FLOAT_EQ(pid.update(2.f, 0.5f, false), 0.f); // seeds last = 2.0
	EXPECT_FLOAT_EQ(pid.update(3.f, 0.f, false), 0.f);  // dt 0 -> forced 0
}

// A-D4 boundary: dt == FLT_EPSILON is NOT > FLT_EPSILON -> guard false.
TEST(PIDStructural, DerivativeGuardExactEpsilonIsNotEnoughTime)
{
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 4.f);
	pid.setSetpoint(0.f);

	EXPECT_FLOAT_EQ(pid.update(2.f, 0.5f, false), 0.f); // seeds last = 2.0
	EXPECT_FLOAT_EQ(pid.update(2.5f, FLT_EPSILON, false), 0.f);
}

// A-D4 just above boundary: dt = 2*EPS passes the guard. Step 0.5 over
// 2*EPS = 2^-22 gives derivative 0.5/2^-22 = 2^21 = 2097152 EXACT, so
// output = +4 * 2097152 (PLUS sign, PID.cpp L47) clamps to the +100 upper
// limit EXACTLY.
TEST(PIDStructural, DerivativeGuardJustAboveEpsilonSaturatesHigh)
{
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 4.f);
	pid.setSetpoint(0.f);

	EXPECT_FLOAT_EQ(pid.update(2.f, 0.5f, false), 0.f); // seeds last = 2.0
	EXPECT_FLOAT_EQ(pid.update(2.5f, 2.f * FLT_EPSILON, false), 100.f);
}

// Invalid-input robustness: negative dt behaves like dt <= EPS (D forced 0),
// leaving the P term alone: error 0.5 * P 2 = 1.0.
TEST(PIDStructural, NegativeDtDisablesDerivativeTerm)
{
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(2.f, 0.f, 4.f);
	pid.setSetpoint(1.f);
	EXPECT_FLOAT_EQ(pid.update(0.5f, -0.5f, false), 1.f);
}

// resetDerivative() sets last back to NaN, so the next D-active update is
// forced to 0 even with valid dt (re-entry into the b=F arm).
TEST(PIDStructural, ResetDerivativeClearsHistory)
{
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 2.f);
	pid.setSetpoint(0.f);

	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, false), 0.f); // seeds last = 0
	pid.resetDerivative();                               // last = NaN again
	EXPECT_FLOAT_EQ(pid.update(1.f, 0.5f, false), 0.f); // forced 0
}

// Integral saturation: I=1, integral limit 0.5, error 1, dt 0.5. Raw integral
// would run 0.5, 1.0, 1.5...; the clamp holds exactly 0.5 (anti-windup upper).
TEST(PIDStructural, IntegralWindupClampsExactlyAtLimit)
{
	PID pid;
	pid.setOutputLimit(10.f);
	pid.setIntegralLimit(0.5f);
	pid.setGains(0.f, 1.f, 0.f);
	pid.setSetpoint(1.f);

	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.f);  // pre-update integral 0
	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.5f); // clamped, not 1.0
	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.5f); // steady, not growing
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.5f);
}

// Output saturation edges: P=1, limit +/-2. Error exactly 2 passes through
// unclamped; error 3 clamps to 2; error 1 passes as 1.
TEST(PIDStructural, OutputLimitPassesExactBoundaryThrough)
{
	PID pid;
	pid.setOutputLimit(2.f);
	pid.setGains(1.f, 0.f, 0.f);
	pid.setSetpoint(5.f);

	EXPECT_FLOAT_EQ(pid.update(3.f, 0.f, false), 2.f); // error exactly 2
	EXPECT_FLOAT_EQ(pid.update(2.f, 0.f, false), 2.f); // error 3 clamps to 2
	EXPECT_FLOAT_EQ(pid.update(4.f, 0.f, false), 1.f); // error 1 passes
}

// Degenerate config: all gains zero -> output stays 0 and the integral path
// (I=0, default integral limit 0) holds 0 without touching the guard.
TEST(PIDStructural, ZeroGainDegenerateConfigHoldsZero)
{
	PID pid;
	pid.setOutputLimit(5.f);
	pid.setGains(0.f, 0.f, 0.f);
	pid.setSetpoint(3.f);

	EXPECT_FLOAT_EQ(pid.update(1.f, 0.1f, true), 0.f);
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.f);
}
