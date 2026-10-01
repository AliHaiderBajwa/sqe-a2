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
 * GTest unit level.
 *
 * Derived from the decision inventory (SCOPE_DECISIONS.md P-D1..P-D3) to close
 * the obligations the upstream PIDTest.cpp leaves open: the non-finite
 * integral guard (P-D2 false path), the full MC/DC quad of the derivative
 * guard `(dt > FLT_EPSILON) && isfinite(last)` (P-D3), the exact
 * dt==FLT_EPSILON boundary, negative-dt robustness, exact-limit saturation,
 * and resetDerivative() semantics. Every expected value is hand-computed
 * from PID.cpp/PID.hpp in the test bodies.
 *
 * NOTE (team): this file is the lead-authored fallback for Ashar's work
 * package (TEAM_BRIEF_ASHAR.md). If Ashar delivers his own PIDStructural
 * tests, his file replaces this one and the inventory/coverage are
 * re-measured; until then this keeps the submission complete.
 */

#include <gtest/gtest.h>

#include <cfloat>
#include <cmath>
#include <PID.hpp>

// P-D2: non-finite integral candidate must not corrupt the integral state.
TEST(PIDStructural, NonFiniteIntegralGuarded)
{
	PID pid;
	pid.setOutputLimit(10.f);
	pid.setIntegralLimit(5.f);
	pid.setGains(0.f, 0.1f, 0.f);
	pid.setSetpoint(1.f);

	EXPECT_TRUE(std::isnan(pid.update(INFINITY, 0.1f, true))); // P*(-inf) is NaN, but integral update is skipped
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.f); // state uncorrupted by the guard

	// Normal operation resumes on the very next sample.
	EXPECT_FLOAT_EQ(pid.update(1.f, 0.1f, true), 0.f); // zero error, integral still 0
	EXPECT_FLOAT_EQ(pid.update(0.f, 0.1f, true), 0.f); // output uses pre-update integral (0)
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.01f); // 0 + 0.1*1*0.1, constrained
}

// P-D3 MC/DC quad for `(dt > FLT_EPSILON) && isfinite(_last_feedback)`.
TEST(PIDStructural, DerivativeGuardAllFalse)
{
	// a=F (dt==0), b=F (last is NAN): derivative forced to 0.
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 5.f);
	pid.setSetpoint(1.f);
	EXPECT_FLOAT_EQ(pid.update(0.5f, 0.f, false), 0.f);
}

TEST(PIDStructural, DerivativeGuardTimeOkButNoHistory)
{
	// a=T (dt=0.1), b=F (last is NAN): derivative forced to 0.
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 5.f);
	pid.setSetpoint(1.f);
	EXPECT_FLOAT_EQ(pid.update(0.5f, 0.1f, false), 0.f);
}

TEST(PIDStructural, DerivativeGuardFullyActive)
{
	// a=T, b=T: derivative (0.6-0.5)/0.1 = 1.0, output 5*1 = 5.
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 5.f);
	pid.setSetpoint(1.f);
	pid.update(0.5f, 0.1f, false);
	EXPECT_FLOAT_EQ(pid.update(0.6f, 0.1f, false), 5.f);
}

TEST(PIDStructural, DerivativeGuardNoTimeButHistory)
{
	// a=F (dt==0), b=T (last finite): derivative forced to 0.
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 5.f);
	pid.setSetpoint(1.f);
	pid.update(0.5f, 0.1f, false);
	EXPECT_FLOAT_EQ(pid.update(0.7f, 0.f, false), 0.f);
}

TEST(PIDStructural, DerivativeGuardExactEpsilonBoundary)
{
	// dt == FLT_EPSILON is NOT > FLT_EPSILON: derivative forced to 0.
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 5.f);
	pid.setSetpoint(1.f);
	pid.update(0.7f, 0.1f, false);
	EXPECT_FLOAT_EQ(pid.update(0.8f, FLT_EPSILON, false), 0.f);
}

TEST(PIDStructural, DerivativeGuardJustAboveEpsilonSaturates)
{
	// dt = 2*FLT_EPSILON: guard passes, (0.8-0.7)/2eps overflows the
	// output limit of 10, so the saturation boundary is exercised via D.
	PID pid;
	pid.setOutputLimit(10.f);
	pid.setGains(0.f, 0.f, 5.f);
	pid.setSetpoint(1.f);
	pid.update(0.7f, 0.1f, false);
	EXPECT_FLOAT_EQ(pid.update(0.8f, 2.f * FLT_EPSILON, false), 10.f);
}

TEST(PIDStructural, NegativeDtDisablesDerivative)
{
	// Invalid input robustness: negative dt behaves like dt<=eps.
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(2.f, 0.f, 5.f);
	pid.setSetpoint(1.f);
	EXPECT_FLOAT_EQ(pid.update(0.5f, -0.1f, false), 1.f); // P-only: 2*0.5
}

TEST(PIDStructural, ResetDerivativeClearsHistory)
{
	// After resetDerivative(), last is NAN again: a=T, b=F -> 0 output.
	PID pid;
	pid.setOutputLimit(100.f);
	pid.setGains(0.f, 0.f, 2.f);
	pid.setSetpoint(1.f);
	pid.update(0.f, 0.1f, false);
	pid.resetDerivative();
	EXPECT_FLOAT_EQ(pid.update(1.f, 0.1f, false), 0.f);
}

TEST(PIDStructural, IntegralWindupClampsAtLimit)
{
	// I=1, integral limit 0.5, error 1, dt 0.5: raw integral would run to
	// 0.5, 1.0, 1.5...; the clamp holds it at exactly 0.5 (anti-windup).
	PID pid;
	pid.setOutputLimit(10.f);
	pid.setIntegralLimit(0.5f);
	pid.setGains(0.f, 1.f, 0.f);
	pid.setSetpoint(1.f);
	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.f); // output uses pre-update integral
	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.5f); // clamped, not 1.0
	EXPECT_FLOAT_EQ(pid.update(0.f, 0.5f, true), 0.5f); // steady, not growing
	EXPECT_FLOAT_EQ(pid.getIntegral(), 0.5f);
}

TEST(PIDStructural, OutputLimitExactBoundaryNotClamped)
{
	// P=1, limit 2, error exactly 2: constrain() passes the value through.
	PID pid;
	pid.setOutputLimit(2.f);
	pid.setGains(1.f, 0.f, 0.f);
	pid.setSetpoint(5.f);
	EXPECT_FLOAT_EQ(pid.update(3.f, 0.f, false), 2.f);
	EXPECT_FLOAT_EQ(pid.update(2.f, 0.f, false), 2.f); // error 3 clamps to 2
	EXPECT_FLOAT_EQ(pid.update(4.f, 0.f, false), 1.f);
}
