# TEAM BRIEF — TAIMOOR (evidence, workbook & report owner)
## SE3002 Assignment 02 — Structural Testing of PX4 Autopilot v1.17.0

> **How to use this file:** give it to your OpenCode/AI as the task prompt, or follow it manually.
> You own everything that turns our code + runs into MARKS: the workbook, the report prose,
> the viva defence pack, and the final package check. You write NO PX4 code — that is the point
> (zero merge conflicts by design).

---

## 1. Your identity in the group

- Group rolls: **24i3102, 24i3052, 24i3072** (tell the lead which roll is yours + your name + section ASAP — needed for filenames `<Roll1_Roll2_Roll3_Section>.<ext>` and the report cover).
- Coordinator (code + build + coverage + packaging): the lead. Ashar: PID unit tests (`src/lib/pid/` only).

## 2. Context you must know (read first)

- SUT: PX4-Autopilot **`v1.17.0`**, commit **`d6f12ad1c4f70ad3230afd7d86e971421e02fef4`**.
- Frozen scope: (1) `src/lib/hysteresis/hysteresis.cpp` — GTest unit; (2) `src/lib/pid/PID.cpp` — GTest unit (Ashar); (3) `src/lib/battery/battery.cpp` — GTest functional + the **MC/DC component** (critical behaviour: battery warning/fault classification driving failsafe; compound decisions at battery.cpp L130 [4-cond], L327–328 [2-cond], L370–371 [3-cond], L375 [2-cond], L377–378 [3-cond]).
- Coverage goal: 100% statement + 100% decision/branch over the scope above (gaps individually justified); MC/DC independence pairs for the Battery compounds; workbook = **exactly 2 sheets**; final judgment = **300–400 words**; no overclaiming ("PX4 is fully tested" = mark loss).
- Full assignment text: ask the lead for `Assignment02-SQE.docx.md` + `REPORT_DRAFT.md` (report skeleton — you fill §§1, 8, 9, 11 and assemble the rest from lead/Ashar inputs).

## 3. Your deliverables (nothing here touches PX4 source)

### A. Testing workbook — `workbook.xlsx` (MOST IMPORTANT, 1 file, ≤2 sheets)
Build in Excel/Sheets, export `.xlsx`. You will receive derivation tables from lead (Hysteresis, Battery) + Ashar (PID).
- **Sheet 1 — Test Inventory**, one row per student test, compact columns: Test ID | component/function | purpose or scenario | key controlled input/state | expected result | execution result (PASS/FAIL/BLOCKED, truthful only) | structural-coverage target (e.g. `PID.cpp L70 T+T`, `hysteresis.cpp L77 c1=T,c2=F`) | test-file/evidence reference.
- **Sheet 2 — MC/DC Evidence**, one row per independence demonstration: compound decision (file:line) | atomic-condition values | overall outcome | independence pair (which two tests) | condition demonstrated independent | source/test reference. Must show every atomic condition T+F and each decision T+F.
- FORBIDDEN: extra sheets (no scope/gap/defect/traceability sheets), long step-by-step procedures (test code is authoritative), invented results (leave execution-result cells pending until real logs arrive).

### B. Report prose (markdown, lead merges into `REPORT_DRAFT.md`)
1. **§1 Introduction + testing approach** (concise, no fluff).
2. **§8 Findings/limitations skeleton** — structure + residual-risk prompts, filled once real results land.
3. **§9 Final quality judgment, 300–400 words** — draft AFTER coverage numbers exist; must state what evidence supports, what is unsupported, confidence level, limits. Word-count it.
4. **§11 AI-assistance record** — collect 5-line notes from all three members into one brief record (uses + assumptions + verification; no chat transcripts).

### C. Viva Q&A bank (markdown, ~20 Q&As)
Every member may be asked: build/run a test, locate a production decision, list atomic conditions, explain an independence pair, justify test level (unit vs functional), interpret a coverage gap, predict coverage lost if a test is removed. Write Q+model-answer for each, covering Hysteresis, PID, Battery, MC/DC, coverage, and gaps. This is shared study material — all three must know it all.

### D. Packaging audit (markdown checklist, verified at the end)
Verify the submission set: report DOCX + own test files + CMake diffs + `git diff` patch vs v1.17.0 + setup/run commands + baseline & final coverage HTML + execution logs + workbook xlsx + AI record; filenames `<Roll1_Roll2_Roll3_Section>.<ext>`; every file opens, every command reproduces.

## 4. Optional (only if you have a Linux machine + time)

- Clone v1.17.0 (hash above), apply our patch files when the lead sends them, rebuild, re-run, and CONFIRM the lead's numbers independently. Any mismatch → report immediately, it is the most valuable defect you can find. Do NOT "fix" code yourself — report back.

## 5. Hard rules

- Never invent test results, coverage %, or defects. Pending = pending.
- Never add workbook sheets or reformat the report skeleton without lead agreement.
- Keep everything traceable: every workbook row points to a real Test ID / real file:line.
- Send the lead: `workbook.xlsx` + the three markdown docs + your name/roll/section + env details (if you built) + your AI-assistance note.
