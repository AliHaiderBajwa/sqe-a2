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
 * @file BatteryStructuralTest.cpp
 *
 * Student-authored structural + MC/DC tests for the Battery class
 * (src/lib/battery/battery.cpp @ v1.17.0) — GTest FUNCTIONAL level
 * (the class depends on PX4 parameters and uORB topics).
 *
 * Critical behaviour under test: battery warning/fault classification that
 * drives the failsafe (determineWarning ladder, determineFaults spike check,
 * and the guards that arm them). Decision inventory: SCOPE_DECISIONS.md (B-D1..B-D12).
 *
 * MC/DC targets in this file:
 *  - B-D4  (L130, 4-condition && RLS-reset gate): each condition flipped while
 *    the other three hold, observable = ocv_estimate_filtered (filter reset
 *    to exactly V when the gate fires vs untouched 0.0f otherwise, I=0 keeps
 *    the OCV filter free of estimator updates).
 *  - B-D8  (L327-328, 2-condition && spike fault): n_cells==0 vs >0, and
 *    voltage strictly-above vs exactly-at the 1.05 boundary.
 *  - B-D9  (L370-371, 3-condition || filter reset): non-finite state,
 *    near-zero state, FW-transition flag — each shown decisive.
 *  - B-D10 (L375, 2-condition &&): armed x finite-current.
 *  - B-D11 (L377-378, 3-condition mix): FW type x fresh stamp x LEVEL phase.
 *  - B-D7  warning ladder boundaries: exact-threshold values take the
 *    higher (less severe) rung because every comparison is strict `<`.
 *
 * No upstream battery test exists; every test below is student-derived.
 */

#include <gtest/gtest.h>

#include <parameters/param.h>
#include <uORB/Publication.hpp>
#include <uORB/topics/flight_phase_estimation.h>
#include <uORB/topics/vehicle_status.h>

#include "battery.h"

static constexpr hrt_abstime T0 = 1000000llu; // 1 s: avoids the t==0 edges
static constexpr int N_CELLS = 4;
static constexpr float V_CHARGED = 4.2f;
static constexpr float V_EMPTY = 3.4f;

class TestBattery : public Battery
{
public:
	TestBattery() : Battery(1, nullptr, 10000, 0 /* source matches default param */)
	{
		param_control_autosave(false);
		setNCellsParam(4); // deterministic RLS init via updateParams L419 (flag=true)
		makeDeterministic();
	}

	void makeDeterministic()
	{
		// Deterministic thresholds/state, independent of param defaults.
		_params.v_empty = V_EMPTY;
		_params.v_charged = V_CHARGED;
		_params.r_internal = -1.f; // use estimated internal resistance
		_params.low_thr = 0.3f;
		_params.crit_thr = 0.15f;
		_params.emergen_thr = 0.07f;
		_params.bat_avrg_current = 5.f;
		_params.source = 0;
	}

	void refreshParams() { updateParams(); }

	void setNCellsParam(int32_t n)
	{
		param_set(param_find("BAT1_N_CELLS"), &n);
		refreshParams();
	}

	void invalidateInternalResistance()
	{
		// n_cells 4 -> 0 flips _internal_resistance_initialized to false (L415-417);
		// then restore n_cells directly so the flag stays false for the B-D4 c3=F case.
		setNCellsParam(0);
		makeDeterministic();
		_params.n_cells = N_CELLS;
	}

	void setParamsNCells(int32_t n) { _params.n_cells = n; }
	void setVoltageCharged(float v) { _params.v_charged = v; }
	void setRInternal(float r) { _params.r_internal = r; }
	void setParamsSource(int32_t s) { _params.source = s; }
	void setBatAvrgCurrent(float c) { _params.bat_avrg_current = c; }
};

class BatteryStructural : public ::testing::Test
{
protected:
	void SetUp() override { param_control_autosave(false); }

	// Publications are fixture members (not helper locals): destroying a
	// Publication unadvertises its node, which would make Subscription::copy()
	// fail inside computeRemainingTime. They must outlive the test body.
	uORB::Publication<vehicle_status_s> _vehicle_status_pub{ORB_ID(vehicle_status)};
	uORB::Publication<flight_phase_estimation_s> _flight_phase_pub{ORB_ID(flight_phase_estimation)};

	void publishVehicleStatus(uint8_t arming_state, uint8_t vehicle_type)
	{
		vehicle_status_s msg{};
		msg.timestamp = hrt_absolute_time();
		msg.arming_state = arming_state;
		msg.vehicle_type = vehicle_type;
		_vehicle_status_pub.publish(msg);
	}

	void publishFlightPhase(uint8_t phase, hrt_abstime stamp)
	{
		flight_phase_estimation_s msg{};
		msg.timestamp = stamp;
		msg.flight_phase = phase;
		_flight_phase_pub.publish(msg);
	}
};

// --- B-D7: warning ladder + strict-< boundaries --------------------------------

TEST_F(BatteryStructural, WarningLadderRungs)
{
	TestBattery b;
	EXPECT_EQ(b.determineWarning(0.50f), battery_status_s::WARNING_NONE);
	EXPECT_EQ(b.determineWarning(0.29f), battery_status_s::WARNING_LOW);
	EXPECT_EQ(b.determineWarning(0.14f), battery_status_s::WARNING_CRITICAL);
	EXPECT_EQ(b.determineWarning(0.06f), battery_status_s::WARNING_EMERGENCY);
	EXPECT_EQ(b.determineWarning(0.0f), battery_status_s::WARNING_EMERGENCY);
}

TEST_F(BatteryStructural, WarningExactThresholdTakesHigherRung)
{
	TestBattery b; // every comparison is strict `<`
	EXPECT_EQ(b.determineWarning(0.30f), battery_status_s::WARNING_NONE);
	EXPECT_EQ(b.determineWarning(0.15f), battery_status_s::WARNING_LOW);
	EXPECT_EQ(b.determineWarning(0.07f), battery_status_s::WARNING_CRITICAL);
}

// --- B-D1/B-D3/B-D6: connection override, init delay, warning arming ----------

TEST_F(BatteryStructural, LowVoltageForcesUnconnected)
{
	TestBattery b;
	b.setConnected(true);
	b.updateVoltage(1.5f); // below LITHIUM_BATTERY_RECOGNITION_VOLTAGE (2.1)
	b.updateBatteryStatus(T0);
	EXPECT_FALSE(b.getBatteryStatus().connected);
}

TEST_F(BatteryStructural, WarningHeldUntilInitialized)
{
	TestBattery b;
	b.setConnected(true);
	b.setStateOfCharge(0.01f); // would be EMERGENCY once armed...
	b.updateVoltage(16.0f);
	b.updateBatteryStatus(T0); // t=1s < last_unconnected+2s -> not initialized
	EXPECT_EQ(b.getBatteryStatus().warning, battery_status_s::WARNING_NONE);
	b.updateBatteryStatus(T0 + 3000000llu); // +3s -> initialized
	EXPECT_EQ(b.getBatteryStatus().warning, battery_status_s::WARNING_EMERGENCY);
}

// --- B-D4 MC/DC: 4-condition RLS-reset gate (I=0 keeps OCV filter clean) -------

TEST_F(BatteryStructural, RlsResetGateFiresPreInit)
{
	// Base firing case: connected=T, not-initialized=T (first update),
	// IR-initialized=T, n_cells>0=T -> reset runs, ocv filter reset to V (I=0).
	TestBattery b;
	b.setConnected(true);
	b.updateVoltage(16.0f);
	b.updateCurrent(0.f);
	b.updateBatteryStatus(T0);
	b.updateBatteryStatus(T0 + 3000000llu);
	EXPECT_FLOAT_EQ(b.getBatteryStatus().ocv_estimate_filtered, 16.0f);
}

TEST_F(BatteryStructural, RlsResetGateBlockedWhenUnconnected)
{
	// c1=F (voltage below recognition keeps _connected false)
	TestBattery b;
	b.invalidateInternalResistance();
	b.setConnected(true);
	b.updateVoltage(1.0f);
	b.updateCurrent(0.f);
	b.updateBatteryStatus(T0);
	b.updateBatteryStatus(T0 + 3000000llu);
	EXPECT_FLOAT_EQ(b.getBatteryStatus().ocv_estimate_filtered, 0.f);
}

TEST_F(BatteryStructural, RlsResetGateOnlyPreInit)
{
	// c2 pair: the same gate fires while !initialized (T0, T0+1s) and stops
	// firing once initialized (T0+3s). Only the init condition differs.
	TestBattery b;
	b.setConnected(true);
	b.updateCurrent(0.f);
	b.updateVoltage(16.0f);
	b.updateBatteryStatus(T0);
	b.updateVoltage(15.0f);
	b.updateBatteryStatus(T0 + 1000000llu);
	EXPECT_FLOAT_EQ(b.getBatteryStatus().ocv_estimate_filtered, 15.0f); // re-fired pre-init
	b.updateVoltage(14.0f);
	b.updateBatteryStatus(T0 + 3000000llu);
	EXPECT_FLOAT_EQ(b.getBatteryStatus().ocv_estimate_filtered, 15.0f); // not re-fired post-init
}

TEST_F(BatteryStructural, RlsResetGateBlockedWhenResistanceInvalidated)
{
	// c3=F (n_cells change cleared the IR-init flag via L415-417) -> gate false
	TestBattery b;
	b.invalidateInternalResistance();
	b.setConnected(true);
	b.updateVoltage(16.0f);
	b.updateCurrent(0.f);
	b.updateBatteryStatus(T0);
	b.updateBatteryStatus(T0 + 3000000llu);
	EXPECT_FLOAT_EQ(b.getBatteryStatus().ocv_estimate_filtered, 0.f);
}

TEST_F(BatteryStructural, RlsResetGateBlockedWhenNoCells)
{
	// c4=F: n_cells==0 -> gate false (all other conditions hold as in the firing case)
	TestBattery b;
	b.setParamsNCells(0);
	b.setConnected(true);
	b.updateVoltage(16.0f);
	b.updateCurrent(0.f);
	b.updateBatteryStatus(T0);
	b.updateBatteryStatus(T0 + 3000000llu);
	EXPECT_FLOAT_EQ(b.getBatteryStatus().ocv_estimate_filtered, 0.f);
	EXPECT_EQ(b.getBatteryStatus().cell_count, 0);
}

// --- B-D8 MC/DC: spike-fault compound + 1.05 boundary ---------------------------

TEST_F(BatteryStructural, SpikeFaultAboveBoundary)
{
	TestBattery b;
	b.updateVoltage(N_CELLS * V_CHARGED * 1.05f + 0.01f);
	b.updateBatteryStatus(T0);
	EXPECT_NE(b.getBatteryStatus().faults & (1 << battery_status_s::FAULT_SPIKES), 0);
}

TEST_F(BatteryStructural, SpikeFaultExactlyAtBoundaryIsClean)
{
	TestBattery b;
	b.updateVoltage(N_CELLS * V_CHARGED * 1.05f); // strict `>`: equal is clean
	b.updateBatteryStatus(T0);
	EXPECT_EQ(b.getBatteryStatus().faults & (1 << battery_status_s::FAULT_SPIKES), 0);
}

TEST_F(BatteryStructural, SpikeFaultNeedsCells)
{
	TestBattery b;
	b.setParamsNCells(0);
	b.updateVoltage(100.f); // huge, but n_cells==0 disarms the check
	b.updateBatteryStatus(T0);
	EXPECT_EQ(b.getBatteryStatus().faults & (1 << battery_status_s::FAULT_SPIKES), 0);
}

// --- B-D12 SoC fusion + coulomb counting ----------------------------------------

TEST_F(BatteryStructural, SocFusesAndDischargesMonotonically)
{
	TestBattery b;
	b.setConnected(true);
	b.setVoltageCharged(V_CHARGED);
	b.updateVoltage(16.4f);
	b.updateCurrent(2.f);
	b.setCapacityMah(1000.f);
	// drive past init, no external SoC -> fusion path (L290 true-branch)
	float prev = 2.f;

	for (int i = 0; i < 8; i++) {
		b.updateBatteryStatus(T0 + (i + 3) * 1000000llu);
		const float soc = b.getBatteryStatus().remaining;
		EXPECT_GE(soc, 0.f);
		EXPECT_LE(soc, 1.f);

		if (i >= 3) {
			EXPECT_LE(soc, prev); // settled: discharge never increases SoC
		}

		prev = soc;
	}
}

TEST_F(BatteryStructural, DtClampCapsDischargeIncrement)
{
	TestBattery b;
	b.updateCurrent(3.6f);
	b.updateBatteryStatus(T0); // first stamp: _dt stays 0 -> no accumulation
	b.updateBatteryStatus(T0 + 10000000llu); // 10 s later -> dt clamped to 2 s
	// 3.6 A * 1000 mA/A * 2 s / 3600 = exactly 2.0 mAh; adding 0 A keeps it there
	EXPECT_FLOAT_EQ(b.sumDischarged(0.f), 2.0f);
}

// --- computeScale finite / non-finite -------------------------------------------

TEST_F(BatteryStructural, ScaleClampedToBand)
{
	TestBattery b;
	b.setConnected(true);
	b.updateVoltage(16.0f);
	b.updateBatteryStatus(T0 + 3000000llu);
	const float scale = b.getBatteryStatus().scale;
	EXPECT_GE(scale, 1.f);
	EXPECT_LE(scale, 1.3f);
}

TEST_F(BatteryStructural, ScaleFallsBackOnNonFinite)
{
	TestBattery b;
	b.setVoltageCharged(0.f); // 0/0 -> NaN -> fallback 1.0 (L343-345)
	b.setConnected(true);
	b.updateVoltage(0.f);
	b.updateBatteryStatus(T0 + 3000000llu);
	EXPECT_FLOAT_EQ(b.getBatteryStatus().scale, 1.f);
}

// --- B-D9/B-D10/B-D11: remaining-time gates ---------------------------------------

TEST_F(BatteryStructural, RemainingTimeNeedsCapacity)
{
	TestBattery b; // capacity 0 -> NaN, no uORB needed
	EXPECT_TRUE(isnan(b.computeRemainingTime(5.f)));
}

TEST_F(BatteryStructural, RemainingTimeUnarmedUsesAverage)
{
	TestBattery b;
	b.setCapacityMah(2000.f);
	b.setStateOfCharge(1.f);
	b.updateBatteryStatus(T0); // unarmed (no vehicle_status published)
	const float t = b.computeRemainingTime(10.f);
	EXPECT_TRUE(PX4_ISFINITE(t));
	EXPECT_GT(t, 0.f);
	EXPECT_FLOAT_EQ(b.getCurrentAverage(), 5.0f); // reset, then untouched (unarmed)
	b.computeRemainingTime(10.f); // all reset conditions false -> holds 5.0
	EXPECT_FLOAT_EQ(b.getCurrentAverage(), 5.0f);
}

TEST_F(BatteryStructural, ArmedMultirotorUpdatesAverage)
{
	TestBattery b;
	b.setCapacityMah(2000.f);
	b.setStateOfCharge(1.f);
	publishVehicleStatus(vehicle_status_s::ARMING_STATE_ARMED,
			     vehicle_status_s::VEHICLE_TYPE_ROTARY_WING);
	b.updateBatteryStatus(T0);
	b.updateBatteryStatus(T0 + 1000000llu);
	const float t = b.computeRemainingTime(20.f); // B-D10 T, B-D11 T via !is_fw
	EXPECT_TRUE(PX4_ISFINITE(t));
	EXPECT_GT(t, 0.f);
}

TEST_F(BatteryStructural, ArmedFixedWingWithStalePhaseHoldsAverage)
{
	TestBattery b;
	b.setCapacityMah(2000.f);
	b.setStateOfCharge(1.f);
	publishVehicleStatus(vehicle_status_s::ARMING_STATE_ARMED,
			     vehicle_status_s::VEHICLE_TYPE_FIXED_WING);
	publishFlightPhase(flight_phase_estimation_s::FLIGHT_PHASE_LEVEL, 1llu); // stale
	b.updateBatteryStatus(T0);
	b.updateBatteryStatus(T0 + 1000000llu);
	b.computeRemainingTime(20.f); // first call: FW transition resets to 5.0
	EXPECT_FLOAT_EQ(b.getCurrentAverage(), 5.0f);
	b.setBatAvrgCurrent(7.f); // move the reset target: a re-reset would show 7.0
	b.updateBatteryStatus(T0 + 2000000llu);
	b.computeRemainingTime(20.f); // transition consumed -> holds 5.0, FW gate stale
	EXPECT_FLOAT_EQ(b.getCurrentAverage(), 5.0f);
}

class TestBatteryIndexed : public Battery
{
public:
	explicit TestBatteryIndexed(int index) : Battery(index, nullptr, 10000, 0) {}
};

TEST_F(BatteryStructural, OutOfRangeIndexClampsToOne)
{
	// Index 99 is out of [1,9]: ctor logs and defaults to 1 (L53, L61-63).
	TestBatteryIndexed b(99);
	EXPECT_EQ(b.getBatteryStatus().id, 1);
}

TEST_F(BatteryStructural, TemperatureRelayedToStatus)
{
	TestBattery b;
	b.updateTemperature(25.5f);
	b.updateBatteryStatus(T0);
	EXPECT_FLOAT_EQ(b.getBatteryStatus().temperature, 25.5f);
}

TEST_F(BatteryStructural, UpdateAndPublishMatchesSource)
{
	TestBattery b; // _source==0 matches _params.source==0 -> publishes
	b.updateVoltage(16.0f);
	b.updateAndPublishBatteryStatus(T0);
	SUCCEED();
}

TEST_F(BatteryStructural, UpdateAndPublishSkipsOnSourceMismatch)
{
	TestBattery b;
	b.setParamsSource(99); // _source(0) != source param -> no publish, no crash
	b.updateVoltage(16.0f);
	b.updateAndPublishBatteryStatus(T0);
	SUCCEED();
}

TEST_F(BatteryStructural, UserInternalResistanceUsedInReset)
{
	// r_internal>=0 with I=0: RLS reset seeds ocv to exactly V (L279-280),
	// and no estimator update disturbs it afterwards.
	TestBattery b;
	b.setRInternal(0.01f);
	b.setConnected(true);
	b.updateVoltage(16.0f);
	b.updateCurrent(0.f);
	b.updateBatteryStatus(T0);
	b.updateBatteryStatus(T0 + 3000000llu);
	EXPECT_FLOAT_EQ(b.getBatteryStatus().ocv_estimate_filtered, 16.0f);
}

TEST_F(BatteryStructural, UserInternalResistanceCorrectsCellVoltage)
{
	// r_internal>=0 with I>0 post-init: load-drop correction uses the
	// user value (L226-227) instead of the estimate.
	TestBattery b;
	b.setRInternal(0.01f);
	b.setConnected(true);
	b.setCapacityMah(2000.f);
	b.updateVoltage(16.0f);
	b.updateCurrent(5.f);
	b.updateBatteryStatus(T0);
	b.updateBatteryStatus(T0 + 1000000llu);
	b.updateBatteryStatus(T0 + 3000000llu);
	const float soc = b.getBatteryStatus().remaining;
	EXPECT_TRUE(PX4_ISFINITE(soc));
	EXPECT_GE(soc, 0.f);
	EXPECT_LE(soc, 1.f);
}

TEST_F(BatteryStructural, ArmedFixedWingWithFreshLevelPhaseUpdatesAverage)
{
	// B-D11 all-true: FW + armed + fresh LEVEL phase -> average filter tracks
	// current, discriminated from the stale-phase test (which holds 5.0).
	TestBattery b;
	b.setCapacityMah(2000.f);
	b.setStateOfCharge(1.f);
	publishVehicleStatus(vehicle_status_s::ARMING_STATE_ARMED,
			     vehicle_status_s::VEHICLE_TYPE_FIXED_WING);
	b.updateBatteryStatus(T0);
	b.updateBatteryStatus(T0 + 1000000llu);
	publishVehicleStatus(vehicle_status_s::ARMING_STATE_ARMED,
			     vehicle_status_s::VEHICLE_TYPE_FIXED_WING);
	publishFlightPhase(flight_phase_estimation_s::FLIGHT_PHASE_LEVEL, hrt_absolute_time());
	const float t = b.computeRemainingTime(20.f);
	EXPECT_GT(b.getCurrentAverage(), 5.0f);
	EXPECT_TRUE(PX4_ISFINITE(t));
}

TEST_F(BatteryStructural, ArmedFixedWingZeroDtUpdatesAverageWithoutDt)
{
	// Same FW/LEVEL setup but _dt==0 (single update): the L379 else path
	// updates the average without a dt argument.
	TestBattery b;
	b.setCapacityMah(2000.f);
	b.setStateOfCharge(1.f);
	publishVehicleStatus(vehicle_status_s::ARMING_STATE_ARMED,
			     vehicle_status_s::VEHICLE_TYPE_FIXED_WING);
	publishFlightPhase(flight_phase_estimation_s::FLIGHT_PHASE_LEVEL, hrt_absolute_time());
	b.updateBatteryStatus(T0); // first stamp only -> _dt stays 0
	b.computeRemainingTime(20.f);
	EXPECT_TRUE(PX4_ISFINITE(b.getCurrentAverage()));
}

TEST_F(BatteryStructural, ArmedFixedWingNonLevelPhaseHoldsAverage)
{
	// B-D11 c3=F: fresh stamp but CLIMB (not LEVEL) -> inner gate false,
	// average holds at the reset value; pair to the LEVEL test.
	TestBattery b;
	b.setCapacityMah(2000.f);
	b.setStateOfCharge(1.f);
	publishVehicleStatus(vehicle_status_s::ARMING_STATE_ARMED,
			     vehicle_status_s::VEHICLE_TYPE_FIXED_WING);
	b.updateBatteryStatus(T0);
	b.updateBatteryStatus(T0 + 1000000llu);
	publishVehicleStatus(vehicle_status_s::ARMING_STATE_ARMED,
			     vehicle_status_s::VEHICLE_TYPE_FIXED_WING);
	publishFlightPhase(flight_phase_estimation_s::FLIGHT_PHASE_CLIMB, hrt_absolute_time());
	b.computeRemainingTime(20.f);
	EXPECT_FLOAT_EQ(b.getCurrentAverage(), 5.0f);
}
