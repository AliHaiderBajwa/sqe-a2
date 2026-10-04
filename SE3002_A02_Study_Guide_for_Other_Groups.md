# SE3002 — Assignment 02: Complete Beginner's Guide (PX4 Structural Testing)

> A from-scratch walkthrough: what the assignment is, every file you must produce, and exactly what to do with each one. Read top to bottom once, then work phase by phase.

---

## TL;DR — what you're actually being graded on

You pick **3 production source files** from PX4-Autopilot v1.17.0, write **your own tests** against them (upstream tests don't count), prove with **coverage** that your tests exercise them fully, prove with an **MC/DC matrix** that one critical component's logic is fully analyzed, and package everything as: **report + workbook + test code + patch + coverage evidence + logs + run instructions + AI record**, all named `<Roll1_Roll2_Roll3_Section>.<extension>`.

Submission ≠ full marks: in the viva, any member may be asked to **build a test, find a line of production code, explain an MC/DC pair, or predict what coverage breaks if a test is deleted**. Everyone must understand everything.

---

## The 8 deliverables — what each file is and what you do with it

### 1. Report — `24i..._C_Report.docx`

**What it is:** the narrative document (the brief says "one concise report").

**What it must contain** (these are the brief's own requirements — use as a checklist):
1. Group details (names + roll numbers + section).
2. Local environment: OS, architecture, compiler/toolchain, the PX4 build/test commands you used.
3. Fixed tag + exact commit hash (`git rev-parse HEAD` output) — all evidence must trace to this.
4. Repository analysis + scope justification: for each of your 3 files — file/class, its responsibility, key dependencies/state, why it's non-trivial, and which PX4 test level you'll use (unit vs functional). No separate scope table needed — prose or bullets.
5. Testing approach: how each test was derived from production logic (begin from the code, not from a desired percentage).
6. MC/DC component selection + interpretation (the atomic conditions, the independence pairs).
7. Coverage analysis: baseline and final numbers in prose (don't just copy the tool's table), every remaining gap identified exactly (statement/branch) with a real justification — "hardware dependent" alone is NOT accepted, and you must not exclude hard logic to boost the percentage.
8. Confirmed defects, if any (file location, repro, expected vs actual). If you found none, say so honestly — there's no minimum defect count. No defect table needed.
9. Remaining gaps / limitations / improvements.
10. **Final judgment, exactly 300–400 words** (count programmatically — it's checked).

**Rules:** evidence-focused — don't duplicate what test code, test output, or the coverage report already show. Keep test-case forms in the workbook, not the report.

**Do with it:** write it LAST, from your logs and coverage files. DOCX format.

---

### 2. Workbook — `24i..._C_Workbook.xlsx` (max 2 sheets, exported from Excel/Google Sheets)

- **Sheet 1 "Test Inventory":** one row per test: TestID, source file/class, test name, what it covers (requirements/decisions/lines/branches), test type/level, priority, preconditions/setup, test procedure, expected result, actual result. (~5 columns minimum; more allowed.)
- **Sheet 2 "MC/DC Evidence":** one row per independence pair: decision ID, atomic condition, condition values (T/F), overall decision outcome, independence pair, demonstrated condition, source/test reference.

**Do with it:** maintain it while you write tests (it's much easier than reconstructing later). **No third sheet** — no scope sheet, no gap sheet, no traceability sheet; those go in the report prose.

---

### 3. Student test source files + CMake changes

**What they are:** your `.cpp` test files under `PX4-Autopilot/src/lib/<your area>/`, plus registration lines in the nearest `CMakeLists.txt`.

**How (the mechanics):**
- Study upstream tests first (`src/lib/*/*Test.cpp`) to learn the macros and naming conventions — but **do not copy or rename them**; the grader diffs against upstream.
- Register via `px4_add_unit_gtest(...)` (for pure libraries) or `px4_add_functional_gtest(...)` (needs PX4 layers, uORB, parameters).
- Each test = fresh object → run → check (setup–run–check). Deterministic inputs (fixed timestamps, controlled params) — no wall-clock, no sleeps.
- Cover: statements, both outcomes of every decision, and edge/boundary/invalid/error cases implied by the source (exact threshold `>=`, NaN/infinity, degenerate values).

**Do with them:** these are uploaded as-is (keep their build paths — don't rename them into the convention; the patch carries the CMake edits).

---

### 4. Patch — `24i..._C_Patch.patch`

**What it is:** `git diff` of your whole change against pristine `v1.17.0` — must apply cleanly with `git apply` on a fresh clone, since you're not uploading the 3.7 GB repo.

**How:**
```bash
git add -A
git diff v1.17.0 -- <your changed files> > 24i..._C_Patch.patch   # or diff the working tree
# verify on a clean checkout:
git stash && git apply --check 24i..._C_Patch.patch
```
**Constraint:** only test files + CMake registrations — **zero production-code edits**.

---

### 5. Setup/run instructions — `RUN_INSTRUCTIONS.md`

Exact, copy-pasteable commands for your machine: clone (fixed tag, `--recursive`), toolchain setup, configure, build, run every test binary, and the coverage commands. Someone else should get green tests + a coverage file from these commands alone.

---

### 6. Coverage evidence — baseline AND final

Two lcov captures of your scope (`hysteresis/pid/battery` — yours will differ):
1. **Baseline** = coverage with ONLY upstream tests (your tests not yet written / not registered).
2. **Final** = coverage with your tests added.

Submit the machine-readable `.info` files **and** the generated **HTML** directory (screenshots alone are insufficient). State the key line/branch results per scope item in the report prose.

Typical flow:
```bash
# instrumented build (use a SEPARATE build dir!)
cmake . -G Ninja -DCONFIG=px4_sitl_test -DCMAKE_BUILD_TYPE=Coverage -B build/px4_sitl_coverage
cd build/px4_sitl_coverage && ninja <your test targets>
<run all your tests>
lcov --capture --branch-coverage --directory . --gcov-tool gcov --ignore-errors mismatch -o cov_all.info
lcov --branch-coverage --extract cov_all.info "*/src/lib/<area1>/*" "*/src/lib/<area2>/*" "*/src/lib/<area3>/*" \
     --ignore-errors mismatch -o scope_coverage_branch.info
genhtml --branch-coverage scope_coverage_branch.info -o html
```
> Environment gotchas we hit (yours may differ): path with **spaces breaks PX4's Makefile** (use a space-free path); `make tests` wrapper misbehaved → configured with cmake directly; `ninja-build` binary name may need a symlink.

**Do with them:** run coverage **twice** (before/after your tests), keep both `.info` + final HTML, quote the numbers in the report, and investigate every gap.

---

### 7. Test execution evidence — run logs

Real output, not screenshots of code. Redirect every run:
```bash
./unit-YourSuite 2>&1 | tee evidence/your_suite_run.log
```
Submit the logs showing **all tests pass** (upstream baseline runs too, if you claim a baseline).

---

### 8. AI-assistance record — `AI_ASSISTANCE_LOG.md`

Brief record: what AI was used for, what material help it gave, and **what assumptions it introduced** (e.g., wrong expected values you had to correct from source). Honest and short; no raw transcripts needed.

---

## The actual workflow, in order

**Phase 0 — Clone & build.**
```bash
git clone --branch v1.17.0 --recursive https://github.com/PX4/PX4-Autopilot.git  # path WITHOUT spaces
cd PX4-Autopilot && git rev-parse HEAD   # record this hash for the report
```
Build the default test config and run upstream tests to prove your env works. Record OS, arch, gcc, cmake, ninja, lcov versions.

**Phase 1 — Scope selection (from the code, not the percentage).**
Browse `src/lib/` for small, self-contained libraries. Good signals: pure logic with branches, deterministic API, no hardware dependency. Bad signals: GUI code, generated code, branch-free math, single-condition wrappers ("superficial" targets are rejected). Pick 3 files. For each, note: responsibility, key dependencies/state, why non-trivial, unit vs functional test level. **Functional level** is required if the code needs PX4 parameters/uORB/time (`px4_add_functional_gtest`).

**Phase 2 — Derive tests from production code.**
Read the file line by line. Every `if`/`&&`/`||` = a decision you must flip both ways. Every boundary (`>=`, `<`, exact constants) = a boundary test at `value` and `value ± 1`. Invalid/error paths (NaN, inf, zero, null) = error tests. Write the inventory row for each test as you write it.

**Phase 3 — MC/DC on ONE component.**
Pick the component whose output drives critical behavior (failsafe/fault/warning logic is ideal). List its compound decisions → break each into atomic conditions → build independence pairs (vary ONE condition, hold the rest, outcome must flip) → implement a test per pair → record every pair in Sheet 2. Trivial decisions can be excluded **with a recorded reason**.

**Phase 4 — Coverage, gaps, iteration.**
Run baseline coverage → add tests → run final coverage. For every uncovered line/branch/condition: name it exactly, then either (a) write a test, or (b) explain precisely why it's unreachable/practical limits, plus what environment/strategy would be needed. Then STOP — don't chase a vanity number.

**Phase 5 — Evidence + writing.**
Freeze logs, generate HTML, then write the report (§1–10 + 300–400 word judgment). Finish the workbook. Generate the patch. Write RUN_INSTRUCTIONS + AI log.

**Phase 6 — Package & self-check.**
- Name everything `<R1_R2_R3_Section>.<extension>` (evidence internals like `.info`/HTML can keep tool names).
- Verify: report has every required section; judgment word count; workbook ≤ 2 sheets; patch applies clean; logs show all-green; commands reproduce everything.
- **Viva prep:** each member must be able to run a test, find the production decision behind it, explain one MC/DC pair, interpret one coverage gap, and say what coverage a deleted test would cost.

---

## Common failure modes (read before you start)

- ❌ Copying/renaming upstream tests → rejected, authorship is diffed.
- ❌ Writing tests to chase a coverage number instead of deriving them from code → rejected.
- ❌ Extra workbook sheets (scope/gap/defect sheets) → "explain in report prose instead".
- ❌ Screenshots instead of HTML/log evidence.
- ❌ "Hardware dependent" as the only gap justification → not accepted.
- ❌ Editing production code to make coverage pass → the patch must show test-only changes.
- ❌ Judgment under 300 / over 400 words.
- ❌ One member unable to explain a pair in the viva → files ≠ marks.

---

## One-line file map

| You create… | Phase | Uploaded as |
|---|---|---|
| test `.cpp` + CMake lines | 1–2 | keep original paths |
| `Workbook.xlsx` | 2–3 | `R1_R2_R3_Sec_Workbook.xlsx` |
| coverage `.info` + HTML | 4 | baseline + final |
| run logs | 4 | per-suite `.log` |
| report | 5 | `R1_R2_R3_Sec_Report.docx` |
| patch | 5 | `R1_R2_R3_Sec_Patch.patch` |
| run instructions + AI record | 5 | `.md` |
