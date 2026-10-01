# Setup / run instructions — PX4 Autopilot v1.17.0 structural-test reproduction

Baseline: tag `v1.17.0`, commit `d6f12ad1c4f70ad3230afd7d86e971421e02fef4`.
Environment used: Linux Mint 22.3 (Ubuntu 24.04 base), x86_64, gcc/g++ 13.3.0,
cmake 3.28.3, ninja 1.11.1, lcov 2.0-1 (gcov 13.3.0), Python 3.12.3, git 2.43.0.

## 1. Clone + toolchain

```bash
git clone --branch v1.17.0 --recursive https://github.com/PX4/PX4-Autopilot.git
cd PX4-Autopilot
git rev-parse HEAD   # expect d6f12ad1c4f70ad3230afd7d86e971421e02fef4
bash Tools/setup/ubuntu.sh   # PX4 toolchain (needs sudo; use a space-free path)
```

Note: the `make tests` wrapper was bypassed on our machine because its
`cmake-build` macro mishandles paths (unquoted `$(SRC_DIR)`); direct
cmake+ninja below is equivalent and is what all evidence was produced with.

## 2. Apply student changes

```bash
git apply /path/to/24i3102_24i3052_24i3072_C_Patch.patch
# adds: src/lib/hysteresis/HysteresisStructuralTest.cpp (+1 CMake line)
#       src/lib/pid/PIDStructuralTest.cpp (+1 CMake line)
#       src/lib/battery/BatteryStructuralTest.cpp (+2 CMake lines, functional)
```

## 3. Build + run (from the repo root)

```bash
cmake . -G Ninja -DCONFIG=px4_sitl_test -B build/px4_sitl_test
cd build/px4_sitl_test
ninja unit-Hysteresis unit-HysteresisStructural unit-PID unit-PIDStructural functional-BatteryStructural
./unit-Hysteresis                  # expect 7/7 PASS (upstream baseline)
./unit-HysteresisStructural        # expect 11/11 PASS (student)
./unit-PID                         # expect 4/4 PASS (upstream baseline)
./unit-PIDStructural               # expect 11/11 PASS (student)
./functional-BatteryStructural     # expect 29/29 PASS (student)
```

## 4. Coverage (instrumented build + lcov, from the repo root)

```bash
cmake . -G Ninja -DCONFIG=px4_sitl_test -DCMAKE_BUILD_TYPE=Coverage -B build/px4_sitl_coverage
cd build/px4_sitl_coverage
ninja unit-Hysteresis unit-HysteresisStructural unit-PID unit-PIDStructural functional-BatteryStructural
./unit-Hysteresis; ./unit-HysteresisStructural; ./unit-PID; ./unit-PIDStructural; ./functional-BatteryStructural
lcov --capture --branch-coverage --directory . --gcov-tool gcov --ignore-errors mismatch -o /tmp/cov_all.info
lcov --branch-coverage --extract /tmp/cov_all.info "*/src/lib/hysteresis/*" "*/src/lib/pid/*" "*/src/lib/battery/*" --ignore-errors mismatch -o scope_coverage_branch.info
genhtml --branch-coverage scope_coverage_branch.info -o html
```

Expected final scope results: `hysteresis.cpp` 22/22 lines, 17/20 arcs
(3 provably unreachable); `PID.cpp` 22/22 lines, 10/10 arcs;
`battery.cpp` 230/231 lines, 19/19 functions (sole gap L72, defensive
`PARAM_INVALID` log unreachable with intact param metadata).
