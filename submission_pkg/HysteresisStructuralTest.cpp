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
 * @file HysteresisStructuralTest.cpp
 *
 * Student-authored structural tests for systemlib::Hysteresis
 * (src/lib/hysteresis/hysteresis.cpp @ v1.17.0).
 *
 * Derived from the decision inventory (SCOPE_DECISIONS.md):
 *  - H-D5/H-D7 MC/DC independence quads for the direction guards
 *    (hysteresis.cpp L77 `if (_state && !_requested_state)` and
 *    L83 `else if (!_state && _requested_state)`).
 *  - H-D6/H-D8 exact-expiry boundary of the `>=` time guards.
 *  - H-D3 repeat-request stamp semantics (re-request must NOT restamp).
 *  - Asymmetric per-direction windows honored independently.
 *
 * These obligations are NOT targeted by the upstream HysteresisTest.cpp
 * (which uses coarse multi-second steps and never probes the exact
 * expiry instant, the pending-vs-quiescent observable, or stamp keeping).
 */

#include <gtest/gtest.h>

#include "hysteresis.h"

static constexpr hrt_abstime T0 = 2000000llu; // arbitrary fixed epoch (us)
static constexpr hrt_abstime WINDOW_TRUE_TO_FALSE = 3000llu;
static constexpr hrt_abstime WINDOW_FALSE_TO_TRUE = 5000llu;

// Bring the object to state=true with a pending true->false request stamped at T0.
// Returns the object; caller advances time.
static systemlib::Hysteresis armed_true_to_false()
{
	systemlib::Hysteresis h(true);
	h.set_hysteresis_time_from(true, WINDOW_TRUE_TO_FALSE);
	h.set_hysteresis_time_from(false, WINDOW_FALSE_TO_TRUE);
	h.set_state_and_update(false, T0); // request stamped at T0, still true
	EXPECT_TRUE(h.get_state());
	return h;
}

// Bring the object to state=false with a pending false->true request stamped at T0.
static systemlib::Hysteresis armed_false_to_true()
{
	systemlib::Hysteresis h(false);
	h.set_hysteresis_time_from(true, WINDOW_TRUE_TO_FALSE);
	h.set_hysteresis_time_from(false, WINDOW_FALSE_TO_TRUE);
	h.set_state_and_update(true, T0); // request stamped at T0, still false
	EXPECT_FALSE(h.get_state());
	return h;
}

// --- H-D5 MC/DC: c1 `_state` independently controls the true->false arm --------

TEST(HysteresisStructural, TrueToFalseArmTakenWhenExpired)
{
	// c1=T, c2=F (requested=F), time expired -> transition occurs (H-D5 T, H-D6 T)
	auto h = armed_true_to_false();
	h.update(T0 + WINDOW_TRUE_TO_FALSE);
	EXPECT_FALSE(h.get_state());
}

TEST(HysteresisStructural, TrueToFalseArmedButWaitingBeforeExpiry)
{
	// c1=T, c2=F, time NOT expired -> armed, no transition yet (H-D5 T, H-D6 F)
	auto h = armed_true_to_false();
	h.update(T0 + WINDOW_TRUE_TO_FALSE - 1);
	EXPECT_TRUE(h.get_state());
}

TEST(HysteresisStructural, QuiescentFalseShowsNoArm)
{
	// c1=F (state=F), c2=F (requested=F): nothing pending (H-D4 F) -> stays F.
	// Observable differs from ArmedButWaiting (T) purely due to c1.
	systemlib::Hysteresis h(false);
	h.set_hysteresis_time_from(true, WINDOW_TRUE_TO_FALSE);
	h.set_hysteresis_time_from(false, WINDOW_FALSE_TO_TRUE);
	h.update(T0 + WINDOW_TRUE_TO_FALSE - 1);
	EXPECT_FALSE(h.get_state());
}

// --- H-D5 MC/DC: c2 `!_requested_state` independently controls the arm ---------

TEST(HysteresisStructural, NoRequestMeansNoTransition)
{
	// c1=T (state=T), c2=T-as-requested==state: request equals state (H-D4 F)
	// -> stays T, vs ArmTakenWhenExpired which ends F. Only c2 differs.
	systemlib::Hysteresis h(true);
	h.set_hysteresis_time_from(true, WINDOW_TRUE_TO_FALSE);
	h.set_hysteresis_time_from(false, WINDOW_FALSE_TO_TRUE);
	h.set_state_and_update(true, T0 + WINDOW_TRUE_TO_FALSE); // no-op request
	h.update(T0 + 2 * WINDOW_TRUE_TO_FALSE);
	EXPECT_TRUE(h.get_state());
}

// --- H-D7 MC/DC: symmetric quad for the false->true guard ---------------------

TEST(HysteresisStructural, FalseToTrueArmTakenWhenExpired)
{
	auto h = armed_false_to_true();
	h.update(T0 + WINDOW_FALSE_TO_TRUE);
	EXPECT_TRUE(h.get_state());
}

TEST(HysteresisStructural, FalseToTrueArmedButWaitingBeforeExpiry)
{
	auto h = armed_false_to_true();
	h.update(T0 + WINDOW_FALSE_TO_TRUE - 1);
	EXPECT_FALSE(h.get_state());
}

TEST(HysteresisStructural, QuiescentTrueShowsNoArm)
{
	systemlib::Hysteresis h(true);
	h.set_hysteresis_time_from(true, WINDOW_TRUE_TO_FALSE);
	h.set_hysteresis_time_from(false, WINDOW_FALSE_TO_TRUE);
	h.update(T0 + WINDOW_FALSE_TO_TRUE - 1);
	EXPECT_TRUE(h.get_state());
}

TEST(HysteresisStructural, NoRequestMeansNoTransitionUpwards)
{
	systemlib::Hysteresis h(false);
	h.set_hysteresis_time_from(true, WINDOW_TRUE_TO_FALSE);
	h.set_hysteresis_time_from(false, WINDOW_FALSE_TO_TRUE);
	h.set_state_and_update(false, T0 + WINDOW_FALSE_TO_TRUE); // no-op request
	h.update(T0 + 2 * WINDOW_FALSE_TO_TRUE);
	EXPECT_FALSE(h.get_state());
}

// --- H-D3: repeat request must NOT restamp the pending transition -------------

TEST(HysteresisStructural, RepeatRequestKeepsOriginalStamp)
{
	auto h = armed_false_to_true(); // stamped at T0, window 5000
	h.set_state_and_update(true, T0 + 4000); // repeat before expiry: stamp kept (H-D3 F-path)
	h.update(T0 + 4999);
	EXPECT_FALSE(h.get_state());
	h.update(T0 + 5000); // original stamp + window -> flips...
	EXPECT_TRUE(h.get_state()); // ...which it would NOT do yet if restamped at T0+4000 (expiry T0+9000)
}

// --- Asymmetric windows honored per direction ----------------------------------

TEST(HysteresisStructural, AsymmetricWindowsIndependent)
{
	// true->false window (3000) must not leak into false->true timing (5000) and back.
	auto h = armed_true_to_false();
	h.update(T0 + WINDOW_TRUE_TO_FALSE); // exactly at true->false expiry
	EXPECT_FALSE(h.get_state());

	// now false; request true at T1 with the 5000 window
	const hrt_abstime T1 = T0 + WINDOW_TRUE_TO_FALSE;
	h.set_state_and_update(true, T1);
	h.update(T1 + WINDOW_TRUE_TO_FALSE); // 3000 < 5000: must still be false
	EXPECT_FALSE(h.get_state());
	h.update(T1 + WINDOW_FALSE_TO_TRUE); // exact 5000 expiry
	EXPECT_TRUE(h.get_state());
}

// --- H-D1: both time registers settable independently --------------------------

TEST(HysteresisStructural, HysteresisTimesSetPerDirection)
{
	systemlib::Hysteresis h(false);
	h.set_hysteresis_time_from(true, 100);
	h.set_hysteresis_time_from(false, 7000);
	h.set_state_and_update(true, T0);
	h.update(T0 + 100); // must NOT flip: false->true window is 7000, not 100
	EXPECT_FALSE(h.get_state());
	h.update(T0 + 7000);
	EXPECT_TRUE(h.get_state());
}
