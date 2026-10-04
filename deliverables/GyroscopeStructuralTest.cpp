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
 * @file GyroscopeStructuralTest.cpp
 *
 * Student-authored structural tests for the Gyroscope calibration class
 * (src/lib/sensor_calibration/Gyroscope.cpp @ v1.17.0) — GTest FUNCTIONAL
 * level (the class depends on PX4 parameters and the sensor_correction
 * uORB topic).
 *
 * Critical behaviour under test: sensor selection, calibration-slot
 * binding, parameter load/save with validity guards, and thermal-offset
 * correction — the chain that decides which calibration a gyro flies with.
 * Decision inventory: G-D1..G-D13 (statement + branch obligations; the
 * compound gates here are simple two-condition guards, so no MC/DC claim
 * is made for this file — the submission MC/DC claim stays on Battery).
 *
 * Device-ID construction: externality is decoded from the bus-type bits by
 * DeviceExternal() (Utilities.cpp). SIMULATION bus -> internal (false),
 * UNKNOWN bus (non-zero id) -> external (true). Both are board-independent,
 * unlike SPI/I2C which depend on board configuration.
 *
 * No upstream sensor_calibration test exists; every test below is
 * student-derived.
 */

#include <gtest/gtest.h>

#include <parameters/param.h>
#include <uORB/Publication.hpp>
#include <uORB/topics/sensor_correction.h>

#include "Gyroscope.hpp"
#include "Utilities.hpp"

using namespace calibration;
using namespace matrix;

// --- deterministic device ids (see DeviceStructure bit layout) ----------------

static uint32_t makeDeviceId(uint8_t bus_type, uint8_t bus, uint8_t address, uint8_t devtype)
{
	return (uint32_t)(bus_type & 0x7u)
	       | ((uint32_t)(bus & 0x1Fu) << 3)
	       | ((uint32_t)address << 8)
	       | ((uint32_t)devtype << 16);
}

// SIMULATION bus (4) -> DeviceExternal() returns false, id is non-zero
static const uint32_t DEV_INTERNAL = makeDeviceId(4, 1, 0x10, 0x20);
static const uint32_t DEV_INTERNAL_2 = makeDeviceId(4, 1, 0x11, 0x20);
// UNKNOWN bus (0), non-zero address -> DeviceExternal() returns true
static const uint32_t DEV_EXTERNAL = makeDeviceId(0, 0, 0x30, 0x40);
static const uint32_t DEV_EXTERNAL_2 = makeDeviceId(0, 0, 0x31, 0x40);

static void setParamInt(const char *name, int32_t value)
{
	param_set(param_find(name), &value);
}

static int32_t getParamInt(const char *name)
{
	int32_t value = 0;
	param_get(param_find(name), &value);
	return value;
}

class GyroscopeStructural : public ::testing::Test
{
protected:
	void SetUp() override
	{
		param_control_autosave(false);
		// snapshot CAL_GYRO0..3 params (tests share one process)
		char name[24];

		for (int i = 0; i < 4; i++) {
			snprintf(name, sizeof(name), "CAL_GYRO%d_ID", i);
			_saved_id[i] = getParamInt(name);
			snprintf(name, sizeof(name), "CAL_GYRO%d_ROT", i);
			_saved_rot[i] = getParamInt(name);
			snprintf(name, sizeof(name), "CAL_GYRO%d_PRIO", i);
			_saved_prio[i] = getParamInt(name);
			snprintf(name, sizeof(name), "CAL_GYRO%d_XOFF", i);
			_saved_off[i](0) = getParamFloat(name);
			snprintf(name, sizeof(name), "CAL_GYRO%d_YOFF", i);
			_saved_off[i](1) = getParamFloat(name);
			snprintf(name, sizeof(name), "CAL_GYRO%d_ZOFF", i);
			_saved_off[i](2) = getParamFloat(name);
			// start from defaults: unbound slots
			snprintf(name, sizeof(name), "CAL_GYRO%d_ID", i);
			setParamInt(name, 0);
		}
	}

	void TearDown() override
	{
		char name[24];

		for (int i = 0; i < 4; i++) {
			snprintf(name, sizeof(name), "CAL_GYRO%d_ID", i);
			setParamInt(name, _saved_id[i]);
			snprintf(name, sizeof(name), "CAL_GYRO%d_ROT", i);
			setParamInt(name, _saved_rot[i]);
			snprintf(name, sizeof(name), "CAL_GYRO%d_PRIO", i);
			setParamInt(name, _saved_prio[i]);
			snprintf(name, sizeof(name), "CAL_GYRO%d_XOFF", i);
			setParamFloat(name, _saved_off[i](0));
			snprintf(name, sizeof(name), "CAL_GYRO%d_YOFF", i);
			setParamFloat(name, _saved_off[i](1));
			snprintf(name, sizeof(name), "CAL_GYRO%d_ZOFF", i);
			setParamFloat(name, _saved_off[i](2));
		}
	}

	static float getParamFloat(const char *name)
	{
		float value = 0.f;
		param_get(param_find(name), &value);
		return value;
	}

	static void setParamFloat(const char *name, float value)
	{
		param_set(param_find(name), &value);
	}

	// Publications are fixture members (not helper locals): destroying a
	// Publication unadvertises its node, which would make
	// Subscription::copy() fail inside SensorCorrectionsUpdate.
	uORB::Publication<sensor_correction_s> _correction_pub{ORB_ID(sensor_correction)};

	void publishCorrection(uint32_t device_id, int slot, float x, float y, float z)
	{
		sensor_correction_s msg{};
		msg.timestamp = hrt_absolute_time();

		for (int i = 0; i < 4; i++) {
			msg.gyro_device_ids[i] = 0;
		}

		msg.gyro_device_ids[slot] = device_id;
		msg.gyro_offset_0[0] = msg.gyro_offset_0[1] = msg.gyro_offset_0[2] = 0.f;
		msg.gyro_offset_1[0] = msg.gyro_offset_1[1] = msg.gyro_offset_1[2] = 0.f;
		msg.gyro_offset_2[0] = msg.gyro_offset_2[1] = msg.gyro_offset_2[2] = 0.f;
		msg.gyro_offset_3[0] = msg.gyro_offset_3[1] = msg.gyro_offset_3[2] = 0.f;

		float *offsets[4] = {msg.gyro_offset_0, msg.gyro_offset_1, msg.gyro_offset_2, msg.gyro_offset_3};
		offsets[slot][0] = x;
		offsets[slot][1] = y;
		offsets[slot][2] = z;
		_correction_pub.publish(msg);
	}

	int32_t _saved_id[4] {};
	int32_t _saved_rot[4] {};
	int32_t _saved_prio[4] {};
	Vector3f _saved_off[4] {};
};

// --- G-D1: construction / Reset -------------------------------------------------

TEST_F(GyroscopeStructural, DefaultConstructorResetsState)
{
	Gyroscope g; // Gyroscope() -> Reset(): internal defaults
	EXPECT_FALSE(g.calibrated()); // device 0, index -1
	EXPECT_EQ(g.device_id(), 0u);
	EXPECT_EQ(g.calibration_index(), -1);
	EXPECT_EQ(g.calibration_count(), 0u);
	EXPECT_FALSE(g.external());
	EXPECT_TRUE(g.enabled()); // default priority 50 > 0
	EXPECT_FLOAT_EQ(g.offset().norm(), 0.f);
	EXPECT_FLOAT_EQ(g.thermal_offset().norm(), 0.f);
}

TEST_F(GyroscopeStructural, DeviceIdConstructorBindsWithoutParams)
{
	Gyroscope g(DEV_INTERNAL); // explicit ctor -> set_device_id -> no saved cal -> Reset
	EXPECT_EQ(g.device_id(), DEV_INTERNAL);
	EXPECT_FALSE(g.external()); // SIMULATION bus
	EXPECT_EQ(g.calibration_index(), -1);
	EXPECT_FALSE(g.calibrated());
}

// --- G-D1/G-D11: set_device_id change vs no-change -------------------------------

TEST_F(GyroscopeStructural, SetDeviceIdInternalSensor)
{
	Gyroscope g;
	g.set_device_id(DEV_INTERNAL);
	EXPECT_TRUE(g.device_id() == DEV_INTERNAL);
	EXPECT_FALSE(g.external());
	EXPECT_EQ(g.priority(), 50); // DEFAULT_PRIORITY after Reset (no saved cal)
}

TEST_F(GyroscopeStructural, SetDeviceIdExternalSensor)
{
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL);
	EXPECT_TRUE(g.external()); // UNKNOWN bus, non-zero id
	EXPECT_EQ(g.priority(), 75); // DEFAULT_EXTERNAL_PRIORITY after Reset
}

TEST_F(GyroscopeStructural, SetDeviceIdSameValueKeepsState)
{
	// L60 false path: same id + same externality -> no Reset -> offset kept
	Gyroscope g;
	g.set_device_id(DEV_INTERNAL);
	EXPECT_TRUE(g.set_offset(Vector3f{0.1f, 0.f, 0.f}));
	g.set_device_id(DEV_INTERNAL); // no change -> must not reset
	EXPECT_FLOAT_EQ(g.offset()(0), 0.1f);
	EXPECT_EQ(g.calibration_count(), 1u);
}

// --- G-D2/G-D3/G-D5/G-D6: thermal correction lookup -------------------------------

TEST_F(GyroscopeStructural, CorrectionIndexZeroApplied)
{
	publishCorrection(DEV_INTERNAL, 0, 0.01f, 0.02f, 0.03f);
	Gyroscope g;
	g.set_device_id(DEV_INTERNAL); // force=true update path
	EXPECT_FLOAT_EQ(g.thermal_offset()(0), 0.01f);
	EXPECT_FLOAT_EQ(g.thermal_offset()(1), 0.02f);
	EXPECT_FLOAT_EQ(g.thermal_offset()(2), 0.03f);
}

TEST_F(GyroscopeStructural, CorrectionIndexThreeApplied)
{
	publishCorrection(DEV_EXTERNAL, 3, -0.05f, 0.06f, -0.07f);
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL);
	EXPECT_FLOAT_EQ(g.thermal_offset()(0), -0.05f);
	EXPECT_FLOAT_EQ(g.thermal_offset()(1), 0.06f);
	EXPECT_FLOAT_EQ(g.thermal_offset()(2), -0.07f);
}

TEST_F(GyroscopeStructural, CorrectionNotFoundZeroesThermal)
{
	publishCorrection(0x00ABCDEFu, 1, 0.5f, 0.5f, 0.5f); // different device
	Gyroscope g;
	g.set_device_id(DEV_INTERNAL);
	EXPECT_FLOAT_EQ(g.thermal_offset().norm(), 0.f); // L107 zero path
}

TEST_F(GyroscopeStructural, NoUpdateWithoutForceOrData)
{
	// L75 false path: nothing published, force=false -> thermal untouched
	Gyroscope g;
	g.SensorCorrectionsUpdate(false);
	EXPECT_FLOAT_EQ(g.thermal_offset().norm(), 0.f);
}

TEST_F(GyroscopeStructural, ZeroDeviceIdSkipsCorrectionCopy)
{
	// L78: force=true but device 0 -> early return, no copy attempted
	publishCorrection(0u, 0, 0.5f, 0.5f, 0.5f);
	Gyroscope g; // device_id 0
	g.SensorCorrectionsUpdate(true);
	EXPECT_FLOAT_EQ(g.thermal_offset().norm(), 0.f);
}

// --- G-D7: set_offset epsilon + finiteness -----------------------------------------

TEST_F(GyroscopeStructural, SetOffsetFirstWriteAccepted)
{
	// count==0 arm (L113 second condition) accepts even a tiny offset
	Gyroscope g;
	EXPECT_TRUE(g.set_offset(Vector3f{0.001f, 0.f, 0.f}));
	EXPECT_EQ(g.calibration_count(), 1u);
	EXPECT_FLOAT_EQ(g.offset()(0), 0.001f);
}

TEST_F(GyroscopeStructural, SetOffsetSameValueRejected)
{
	// diff below 0.01 and count>0 -> L113 false -> return false
	Gyroscope g;
	EXPECT_TRUE(g.set_offset(Vector3f{0.1f, 0.f, 0.f}));
	EXPECT_FALSE(g.set_offset(Vector3f{0.1f, 0.f, 0.f}));
	EXPECT_EQ(g.calibration_count(), 1u);
}

TEST_F(GyroscopeStructural, SetOffsetLargeChangeAccepted)
{
	Gyroscope g;
	EXPECT_TRUE(g.set_offset(Vector3f{0.1f, 0.f, 0.f}));
	EXPECT_TRUE(g.set_offset(Vector3f{0.5f, 0.f, 0.f})); // longerThan(0.01)
	EXPECT_EQ(g.calibration_count(), 2u);
}

TEST_F(GyroscopeStructural, SetOffsetRejectsNonFinite)
{
	// L114 false path: non-finite offset never stored
	Gyroscope g;
	const float nan = NAN;
	EXPECT_FALSE(g.set_offset(Vector3f{nan, 0.f, 0.f}));
	EXPECT_EQ(g.calibration_count(), 0u);
	EXPECT_FLOAT_EQ(g.offset().norm(), 0.f);
}

// --- G-D8: calibration index bounds -------------------------------------------------

TEST_F(GyroscopeStructural, SetCalibrationIndexBounds)
{
	Gyroscope g;
	EXPECT_TRUE(g.set_calibration_index(2));
	EXPECT_EQ(g.calibration_index(), 2);
	EXPECT_FALSE(g.set_calibration_index(-1));
	EXPECT_FALSE(g.set_calibration_index(4)); // MAX_SENSOR_COUNT
	EXPECT_EQ(g.calibration_index(), 2); // unchanged by rejections
}

// --- G-D9/G-D10: ParametersUpdate / ParametersLoad ------------------------------------

TEST_F(GyroscopeStructural, ParametersUpdateNoDeviceReturnsEarly)
{
	// L144: device 0 -> return, index untouched
	Gyroscope g;
	g.ParametersUpdate();
	EXPECT_EQ(g.calibration_index(), -1);
}

TEST_F(GyroscopeStructural, ParametersLoadExternalValid)
{
	// Full Load path: bound slot 0, valid ROT/PRIO/OFF
	setParamInt("CAL_GYRO0_ID", (int32_t)DEV_EXTERNAL);
	setParamInt("CAL_GYRO0_ROT", 2);
	setParamInt("CAL_GYRO0_PRIO", 50);
	setParamFloat("CAL_GYRO0_XOFF", 0.11f);
	setParamFloat("CAL_GYRO0_YOFF", -0.12f);
	setParamFloat("CAL_GYRO0_ZOFF", 0.13f);
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL);
	EXPECT_EQ(g.calibration_index(), 0);
	EXPECT_TRUE(g.calibrated());
	EXPECT_EQ(g.rotation_enum(), static_cast<Rotation>(2));
	EXPECT_EQ(g.priority(), 50);
	EXPECT_FLOAT_EQ(g.offset()(0), 0.11f);
	EXPECT_FLOAT_EQ(g.offset()(1), -0.12f);
	EXPECT_FLOAT_EQ(g.offset()(2), 0.13f);
}

TEST_F(GyroscopeStructural, InvalidRotationResetToNone)
{
	// L166: external + ROT=99 (>= ROTATION_MAX) -> ROTATION_NONE
	setParamInt("CAL_GYRO0_ID", (int32_t)DEV_EXTERNAL);
	setParamInt("CAL_GYRO0_ROT", 99);
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL);
	EXPECT_EQ(g.rotation_enum(), ROTATION_NONE);
}

TEST_F(GyroscopeStructural, InvalidPriorityResetToDefault)
{
	// L181/L185: PRIO=150 out of range and != -1 -> param reset, default used
	setParamInt("CAL_GYRO0_ID", (int32_t)DEV_EXTERNAL);
	setParamInt("CAL_GYRO0_PRIO", 150);
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL);
	EXPECT_EQ(getParamInt("CAL_GYRO0_PRIO"), -1); // reset by Load
	EXPECT_EQ(g.priority(), 75); // DEFAULT_EXTERNAL_PRIORITY
}

TEST_F(GyroscopeStructural, InternalSensorFollowsBoardRotation)
{
	// L173-176: internal sensors ignore the ROT param, use board rotation
	setParamInt("CAL_GYRO0_ID", (int32_t)DEV_INTERNAL);
	setParamInt("CAL_GYRO0_ROT", 2); // must be ignored
	Gyroscope g;
	g.set_device_id(DEV_INTERNAL);
	EXPECT_EQ(g.rotation_enum(), GetBoardRotation());
}

// --- G-D12: ParametersSave --------------------------------------------------------------

TEST_F(GyroscopeStructural, ParametersSaveForcedSlot)
{
	// L227 true path: force + valid desired index -> direct bind + save
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL);
	EXPECT_TRUE(g.ParametersSave(1, true));
	EXPECT_EQ(g.calibration_index(), 1);
	EXPECT_EQ(getParamInt("CAL_GYRO1_ID"), (int32_t)DEV_EXTERNAL);
}

TEST_F(GyroscopeStructural, ParametersSaveFindsNewSlotWithWarning)
{
	// L230/L237: previously bound index 0 no longer matches -> slot 1 + warn path
	setParamInt("CAL_GYRO0_ID", (int32_t)DEV_EXTERNAL);
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL);
	EXPECT_EQ(g.calibration_index(), 0);
	setParamInt("CAL_GYRO0_ID", (int32_t)DEV_EXTERNAL_2); // slot 0 taken by other
	EXPECT_TRUE(g.ParametersSave(-1, false));
	EXPECT_EQ(g.calibration_index(), 1); // first free slot
}

TEST_F(GyroscopeStructural, ParametersSaveNoSlotFails)
{
	// L243 false path: all slots bound to other devices -> index -1 -> false
	setParamInt("CAL_GYRO0_ID", (int32_t)DEV_EXTERNAL);
	setParamInt("CAL_GYRO1_ID", (int32_t)DEV_EXTERNAL_2);
	setParamInt("CAL_GYRO2_ID", (int32_t)DEV_INTERNAL);
	setParamInt("CAL_GYRO3_ID", (int32_t)DEV_INTERNAL_2);
	Gyroscope g;
	g.set_device_id(0x00F00D00u); // bound nowhere (UNKNOWN bus -> external)
	EXPECT_FALSE(g.ParametersSave(-1, false));
	EXPECT_EQ(g.calibration_index(), -1);
}

// --- inline math: Correct / Uncorrect / BiasCorrectedSensorOffset --------------------------

TEST_F(GyroscopeStructural, CorrectUncorrectRoundTrip)
{
	Gyroscope g;
	g.set_device_id(DEV_INTERNAL);
	EXPECT_TRUE(g.set_offset(Vector3f{0.1f, -0.2f, 0.3f}));
	const Vector3f raw{1.f, 2.f, 3.f};
	const Vector3f corrected = g.Correct(raw);
	const Vector3f back = g.Uncorrect(corrected);
	EXPECT_FLOAT_EQ(back(0), raw(0));
	EXPECT_FLOAT_EQ(back(1), raw(1));
	EXPECT_FLOAT_EQ(back(2), raw(2));
	// offset actually applied: corrected == R*(raw - thermal - offset)
	EXPECT_TRUE((corrected - (g.rotation() * (raw - g.offset()))).norm() < 1e-5f);
}

TEST_F(GyroscopeStructural, BiasCorrectedSensorOffsetMath)
{
	Gyroscope g;
	g.set_device_id(DEV_INTERNAL);
	EXPECT_TRUE(g.set_offset(Vector3f{0.1f, 0.f, 0.f}));
	const Vector3f bias{0.01f, 0.02f, 0.03f};
	const Vector3f expected = g.offset() + (g.rotation().T() * bias);
	const Vector3f got = g.BiasCorrectedSensorOffset(bias);
	EXPECT_TRUE((got - expected).norm() < 1e-6f);
}

// --- G-D13: PrintStatus ----------------------------------------------------------------------

TEST_F(GyroscopeStructural, PrintStatusBothPaths)
{
	// Logging only; exercises external/internal + thermal print branches
	publishCorrection(DEV_EXTERNAL, 1, 0.01f, 0.f, 0.f);
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL);
	g.PrintStatus(); // external + thermal>0
	Gyroscope h;
	h.set_device_id(DEV_INTERNAL);
	h.PrintStatus(); // internal, thermal zero
}

// --- gap-closure probes (coverage round 2) ------------------------------------------

TEST_F(GyroscopeStructural, CorrectionSlotTwoApplied)
{
	// Switch case 2 (L95-97): the only correction slot not yet exercised
	publishCorrection(DEV_INTERNAL_2, 2, 0.04f, -0.05f, 0.06f);
	Gyroscope g;
	g.set_device_id(DEV_INTERNAL_2);
	EXPECT_FLOAT_EQ(g.thermal_offset()(0), 0.04f);
	EXPECT_FLOAT_EQ(g.thermal_offset()(1), -0.05f);
	EXPECT_FLOAT_EQ(g.thermal_offset()(2), 0.06f);
}

TEST_F(GyroscopeStructural, ParametersLoadRejectedIndexDirect)
{
	// L161 first-operand false + L201: index -1 straight into Load -> false
	Gyroscope g; // Reset left index at -1
	EXPECT_FALSE(g.ParametersLoad());
	EXPECT_EQ(g.calibration_index(), -1);
}

TEST_F(GyroscopeStructural, SaveForcedNegativeDesiredFindsSlot)
{
	// L227 false via desired<0, then L230-231 evaluated with force=true:
	// elif false (slot already matches) -> straight to save
	setParamInt("CAL_GYRO0_ID", (int32_t)DEV_EXTERNAL);
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL);
	EXPECT_EQ(g.calibration_index(), 0);
	EXPECT_TRUE(g.ParametersSave(-1, true));
	EXPECT_EQ(g.calibration_index(), 0); // slot kept, no warning
	EXPECT_EQ(getParamInt("CAL_GYRO0_ID"), (int32_t)DEV_EXTERNAL);
}

TEST_F(GyroscopeStructural, SaveInternalWritesRotationMinusOne)
{
	// elif-true with matching slot (L237 no-warn) + internal ROT=-1 save (L253-254)
	setParamInt("CAL_GYRO0_ID", (int32_t)DEV_INTERNAL);
	Gyroscope g;
	g.set_device_id(DEV_INTERNAL);
	EXPECT_TRUE(g.ParametersSave(-1, false)); // slot 0 matches -> no index change
	EXPECT_EQ(g.calibration_index(), 0);
	EXPECT_TRUE(g.ParametersSave(0, true));
	EXPECT_EQ(getParamInt("CAL_GYRO0_ROT"), -1); // internal marker
	EXPECT_EQ(getParamInt("CAL_GYRO0_ID"), (int32_t)DEV_INTERNAL);
}

TEST_F(GyroscopeStructural, UnadvertisedCorrectionSkipsCopy)
{
	// uORB failure injection: publish, unadvertise, then a fresh gyro
	// observes force=true with copy()=false (L84 false path) and keeps
	// a zero thermal offset instead of touching the loop.
	publishCorrection(DEV_EXTERNAL, 0, 0.5f, 0.5f, 0.5f);
	_correction_pub.unadvertise();
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL);
	EXPECT_FLOAT_EQ(g.thermal_offset().norm(), 0.f);
}

TEST_F(GyroscopeStructural, SaveOutOfRangeDesiredFallsBackToFreeSlot)
{
	// L227 third-operand false (desired=4 >= MAX) with force=true, then the
	// L231 third operand takes the FindAvailable path; the out-of-range
	// preference is ignored and the first free slot is used.
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL_2);
	EXPECT_TRUE(g.ParametersSave(4, true));
	EXPECT_EQ(g.calibration_index(), 0);
	EXPECT_EQ(getParamInt("CAL_GYRO0_ID"), (int32_t)DEV_EXTERNAL_2);
}

TEST_F(GyroscopeStructural, SaveUnboundNegativeDesiredFindsSlot)
{
	// L230 second operand true with force=true: unbound device (index -1)
	// still resolves through FindAvailable to the first free slot.
	Gyroscope g; // device 0, index -1
	EXPECT_TRUE(g.ParametersSave(-1, true));
	EXPECT_EQ(g.calibration_index(), 0);
}

TEST_F(GyroscopeStructural, SaveOutOfRangeDesiredKeepsBoundSlot)
{
	// L231 fully evaluated with force=true: desired=4 is not -1 and differs
	// from the bound index, so the elif takes the FindAvailable path — which
	// returns the already-bound slot 0 instead of the invalid preference.
	setParamInt("CAL_GYRO0_ID", (int32_t)DEV_EXTERNAL_2);
	Gyroscope g;
	g.set_device_id(DEV_EXTERNAL_2);
	EXPECT_EQ(g.calibration_index(), 0);
	EXPECT_TRUE(g.ParametersSave(4, true));
	EXPECT_EQ(g.calibration_index(), 0);
	EXPECT_EQ(getParamInt("CAL_GYRO0_ID"), (int32_t)DEV_EXTERNAL_2);
}
