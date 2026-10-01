# TEAM BRIEF — ASHAR (PID unit-test owner)
## SE3002 Assignment 02 — Structural Testing of PX4 Autopilot v1.17.0

> **How to use this file:** give it to your OpenCode/AI as the task prompt, or follow it manually.
> Your work is ONE isolated piece of a group submission. Stay inside your boundaries —
> anything outside them belongs to another member and will cause merge conflicts.

---

## 1. Your identity in the group

- Group rolls: **24i3102, 24i3052, 24i3072** (tell the lead which roll is yours + your name + section ASAP — needed for filenames `<Roll1_Roll2_Roll3_Section>.<ext>`).
- Coordinator (integration + final packaging): the lead with this repo. Report + workbook + Battery: lead/Taimoor (see §3).

## 2. Fixed baseline (do not change)

- Repo: `https://github.com/PX4/PX4-Autopilot`, branch/tag **`v1.17.0`**, commit **`d6f12ad1c4f70ad3230afd7d86e971421e02fef4`** (verify with `git rev-parse HEAD` after cloning).
- Clone: `git clone --branch v1.17.0 --recursive https://github.com/PX4/PX4-Autopilot.git` (needs ~3 GB + submodules; all 29 submodules must init).
- Env: Ubuntu 22.04/24.04 (or WSL2/Mint equivalent). Install: `cmake`, `ninja-build` (note: Ubuntu package `ninja-build` provides the `ninja` binary; PX4's Makefile looks for `ninja-build`, so run `sudo ln -sf /usr/bin/ninja /usr/local/bin/ninja-build`), `lcov`, then `bash Tools/setup/ubuntu.sh` (if your path contains a space, run it via a space-free symlink — ask lead if it fails at the pip step).
- **Known build quirk:** the top-level `make tests` wrapper is fragile (unquoted paths); configure/build directly:
  ```bash
  cd PX4-Autopilot
  mkdir -p build/px4_sitl_test
  cmake . -G Ninja -DCONFIG=px4_sitl_test -B build/px4_sitl_test   # ~1 min, one-time
  cd build/px4_sitl_test && ninja unit-PID                          # your target (after §4 edit)
  ./unit-PID                                                        # run
  ```
- Record YOUR env (OS, arch, gcc version, cmake/ninja versions) and send it to the lead for the report.

## 3. Your scope — and ONLY your scope

- **YOU OWN: `src/lib/pid/PID.cpp` (+ `PID.hpp` for reference). Test level: GTest UNIT.**
  - Why: control-critical PID (output/integral saturation, anti-windup, NaN/dt guards), deterministic, internal-only deps.
- **DO NOT TOUCH:** `src/lib/hysteresis/` (lead), `src/lib/battery/` (lead), any other `src/` file, any existing test file. The ONLY files you may create/modify:
  1. CREATE `src/lib/pid/PIDStructuralTest.cpp` (your tests; name must differ from upstream `PIDTest.cpp`)
  2. MODIFY `src/lib/pid/CMakeLists.txt` (add exactly one line: `px4_add_unit_gtest(SRC PIDStructuralTest.cpp LINKLIBS PID)`)
- **Authorship rule (viva-critical):** read upstream `PIDTest.cpp` to understand conventions, but NEVER copy/rename its tests. Your tests must be derived from YOUR OWN decision analysis below. During viva you may be asked to explain any test or predict coverage loss if a test is removed.

## 4. Step-by-step work

1. **Read** `PID.cpp` (75 lines) + `PID.hpp`. List EVERY decision: `PID::update` (`if update_integral`), `updateIntegral` (`if isfinite`), `updateDerivative` (`if (dt > FLT_EPSILON) && isfinite(last)` — the one compound decision), plus all `math::constrain` saturation boundaries.
2. **Derive obligations** (this is what earns Part 2 marks — document as a small table you send the lead):
   - Statement coverage: which tests execute which lines.
   - Branch coverage: BOTH outcomes of each `if` (esp. integral-update on/off, finite/non-finite integral, dt above/at-or-below `FLT_EPSILON` × last-feedback finite/NaN).
   - Boundary/invalid/error cases upstream `PIDTest.cpp` does NOT cover — verify by reading it: at minimum **NaN feedback, ±Inf feedback/error (non-finite integral path), `dt = FLT_EPSILON` vs just-above, negative dt, integral exactly at limit, output exactly at limit, `resetDerivative()` (sets last=NaN) then D-active update, zero-gain + zero-limit degenerate config**. Each gets a test with a hand-computed expected value (compute by hand from the source, never trust AI output blindly).
   - MC/DC-style independence pair for the `updateDerivative` compound: hold `isfinite(last)` true while flipping `dt > EPS`, then hold `dt > EPS` true while flipping finiteness (via `resetDerivative()`), showing the outcome change each time.
3. **Implement** `PIDStructuralTest.cpp` (GTest `TEST(PIDStructural, ...)` cases, deterministic, no ordering dependence, `EXPECT_FLOAT_EQ`/`EXPECT_NEAR` with exact expectations). Build with `ninja unit-PIDStructural`… (binary will be named `unit-PIDStructural` per the `px4_add_unit_gtest` rule: filename minus `Test`).
4. **Run** the binary; all your tests must PASS (never manufacture failures; if one FAILS, investigate setup/expected-value first — a genuine reproducible defect must be reported with location + repro + expected vs actual).
5. **Coverage:** lead owns the global lcov run; you report per-test contribution qualitatively (which lines/branches each test hits, by reasoning) — or run `gcov` on `PID.cpp` yourself if able and send the numbers.

## 5. Deliverables — send ALL to the lead

1. `PIDStructuralTest.cpp` (your file) + the one-line CMake diff.
2. Your derivation table (obligation → Test ID → expected result) as markdown.
3. Full run log (`./unit-PIDStructural` output, copy-paste).
4. Your env details (OS/arch/gcc/cmake/ninja) + a 5-line AI-assistance note (what AI you used, what assumptions it introduced, how you verified them).
5. Your name + roll + section.

## 6. Hard rules

- No production-code changes (`PID.cpp`/`PID.hpp` are READ-ONLY for you).
- No new dependencies, no test-order dependence, no `as any`-style shortcuts — exact float expectations only.
- Keep tests fast (<1 s total). Ask the lead before adding ANY file outside §3.
