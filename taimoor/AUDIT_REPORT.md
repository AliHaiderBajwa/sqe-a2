# AUDIT_REPORT — SE3002 A2 Consistency Audit (Taimoor track)
Date: 2026-10-01. Auditor role: Taimoor 24i3052. Baseline: PX4 v1.17.0 @ d6f12ad1c4f70ad3230afd7d86e971421e02fef4.
Rule followed: no PX4/test/patch/evidence files modified; no numbers invented; UNVERIFIED labelled.

Sources checked: `sqe-a2/Assignment02-SQE.docx.md`, `PROGRESS_LOG.md`, `SCOPE_DECISIONS.md`,
`TEST_INVENTORY.md`, `MCDC_MATRIX.md`, `REPORT_DRAFT.md`, `AI_ASSISTANCE_LOG.md`,
`TEAM_BRIEF_TAIMOOR.md`, `TEAM_BRIEF_ASHAR.md`, `workbook/Workbook.xlsx`,
`submission_pkg/24i3102_24i3052_24i3072_C_Workbook.xlsx`,
`evidence/baseline/scope_baseline_branch.info`, `evidence/final/scope_coverage_branch.info`,
`evidence/final/html/`, `evidence/logs/student_*_run.log`, `evidence/baseline/unit-*_run.log`,
`patch/full_reconstruction.patch`, `patch/student_changes.diff`, `submission_pkg/`,
local clone `px4-autopilot/` @ v1.17.0, `git ls-remote` peeled tag.

## 0. Verified-good (no fix needed)
- Commit hash CONSISTENT everywhere and evidence-backed: `git ls-remote` peeled
  `refs/tags/v1.17.0^{}` = `d6f12ad1c4f70ad3230afd7d86e971421e02fef4`;
  local `px4-autopilot` HEAD = same; `REPORT_DRAFT.md:17,33,106`,
  `PROGRESS_LOG.md:34,53`, `submission_pkg/RUN_INSTRUCTIONS.md:3,12` all state same hash. PASS.
- Test counts: patch has 51 `+TEST` (29 Battery + 11 Hysteresis + 11 PID);
  workbook Sheet1 has 51 data rows (11 HST + 29 BST + 11 PST); run logs show
  11/11 + 29/29 + 11/11 PASS. No orphan rows: every workbook ID maps to a real
  `TEST` name in patch and a `[ RUN ]` line in logs. PASS on counts.
- Execution results: workbook col F = all `PASS`; each is proven by
  `evidence/logs/student_{HysteresisStructural,BatteryStructural,PIDStructural}_run.log`
  and identical copies in `submission_pkg/`. PASS.
- Workbook sheet count: exactly 2 visible sheets (`Test Inventory` 52 rows incl header,
  `MC_DC Evidence` 33 rows incl header); no hidden sheets (`sheet_state` = visible for both).
  `submission_pkg/..._Workbook.xlsx` identical dimensions. PASS on count.
- Patch applies cleanly: `git -C px4-autopilot apply --check patch/full_reconstruction.patch` → all 6 files OK.
- Baseline contribution story VERIFIED: baseline `.info` contains NO battery file (0% — no upstream
  test links it); hysteresis.cpp baseline already 22/22 + 17/20; PID.cpp baseline 22/22 + 8/10.
  Final `.info` = hysteresis 22/22 + 17/20, PID 22/22 + 10/10, battery 230/231 + 170/248.
  So "battery 0→99.6%" and "PID 8/10→10/10 via fallback" are both true at different times.
- Environment block identical in `REPORT_DRAFT.md:35-36` and `RUN_INSTRUCTIONS.md`
  (Mint 22.3 / x86_64 / gcc 13.3.0 / cmake 3.28.3 / ninja 1.11.1 / lcov 2.0-1 / Python 3.12.3 / git 2.43.0). PASS.

## 1. Findings (each: file/location, conflict, evidence-backed source, exact fix)

### F1. Stale DOCX filename in PROGRESS_LOG (must-fix, trivial)
- `PROGRESS_LOG.md:9` says DOCX: `24i3102_24i3052_24i3072_SECTION-TBD_Report.docx`.
- Actual files: `sqe-a2/24i3102_24i3052_24i3072_C_Report.docx` and
  `sqe-a2/submission_pkg/24i3102_24i3052_24i3072_C_Report.docx` (both present, 46K).
- Evidence-backed: filesystem listing. Fix: edit `PROGRESS_LOG.md:9` to
  `24i3102_24i3052_24i3072_C_Report.docx`.

### F2. PID "8/10 vs 10/10" — timeline, not contradiction, but must annotate (should-fix)
- `PROGRESS_LOG.md:14` (milestone entry): "PID.cpp 22/22, 8/10 (2 pending Ashar's NaN/Inf tests)".
- `PROGRESS_LOG.md:8` (newest entry) + `REPORT_DRAFT.md:80` + `RUN_INSTRUCTIONS.md`: "arcs 10/10".
- Evidence-backed: baseline `.info` PID BRF/BRH = 10/8 (proves 8/10 was true pre-fallback);
  final `.info` PID BRF/BRH = 10/10 (proves 10/10 is current truth).
- Fix: append "(superseded — pre-fallback; see newest entry 10/10)" to the line-14 entry.
  Do NOT delete history. Answer to brief: final 10/10 is true; 8/10 was the upstream-only baseline.

### F3. PID authorship wording — unify to AI-drafted/lead-verified fallback (must-fix honesty)
- `TEST_INVENTORY.md:72`, `MCDC_MATRIX.md:91`, `REPORT_DRAFT.md:57` say "lead-authored fallback".
- `AI_ASSISTANCE_LOG.md:9` says "drafted `PIDStructuralTest.cpp` (11 tests) to Ashar's brief spec"
  by AI, "verified" by 11/11 PASS + 10/10 arcs. `TEAM_BRIEF_ASHAR.md` proves Ashar was assigned
  PID but no Ashar-authored file was delivered.
- Evidence-backed: AI log + absence of Ashar file + patch containing the 11 PST tests.
- Truth: Ashar authored NOTHING in the submission; the file is AI-drafted (Sisyphus/OpenCode)
  to Ashar's spec, human-verified. Fix in all four places: replace "lead-authored fallback"
  with "AI-drafted, lead-verified fallback (Ashar delivered no file; swap + re-measure if it arrives)".
  `REPORT_DRAFT.md:126` already admits AI scaffolding — keep that sentence.

### F4. Failure count "2 vs 3" — UNVERIFIED, must reconcile (must-fix)
- `PROGRESS_LOG.md:16,22` + `TEST_INVENTORY` context: "2 early Battery failures ... test-side expectation errors on L130 gate".
- `REPORT_DRAFT.md:56,88`: "3 student-test failures ... (1) two RLS-gate timings + (2) PID NaN output value".
- Evidence: NO failing-test log is preserved; all current logs are green. Neither count is
  evidence-backed beyond prose. Fix: pick the true count from the author (Ali) and either
  (a) keep "3" and add one line to PROGRESS_LOG describing the PID NaN expectation fix
  (`expected 0.0, actual NaN; P*(-Inf) is NaN, integral guard held`), or (b) correct report to "2".
  Until confirmed, treat the third failure as UNVERIFIED.

### F5. Workbook Sheet2 MISSING required column (must-fix per assignment p.3)
- Assignment (`Assignment02-SQE.docx.md:139`): Sheet2 must record "atomic-condition values,
  overall decision outcome, independence pair, condition demonstrated independent, AND source/test reference".
- `Workbook.xlsx` Sheet2 headers (5 cols): Compound decision | Atomic-condition values |
  Overall outcome | Independence pair | Condition demonstrated. NO source/test-reference column.
- Evidence-backed: `openpyxl` header read. Fix: add col F "Source / test reference"
  (e.g. `battery.cpp:130 / BST-05 vs BST-06`) to the proposed copy only
  (`Workbook_proposed.xlsx`); NEVER overwrite `workbook/Workbook.xlsx` in place.
  Sheet1 (8 cols) already matches the required 8 — no fix.

### F6. Workbook Sheet2 contains 2 non-evidence rows counted as evidence (should-fix)
- Sheet2 has 32 data rows, but row 22 (B-D9 c1: pair `—`, outcome `(see gap note)`) and
  row 25 (B-D10 c2: pair `(NaN probe, removed — ...)`) are gap notes, not test pairs.
- Strict demonstrated pairs = 30. Fix: in proposed copy, move these two rows to a
  clearly labelled "Investigated gaps (not counted as pairs)" block or add a Status column
  (DEMONSTRATED vs GAP-ANALYSED). Do not claim "32 pairs".

### F7. Workbook Ref column uses "..." truncation (should-fix)
- Sheet1 col H uses `...BlockedWhenUnconnected`, `...OnlyPreInit`, etc. for ~20 rows
  instead of full `Suite.Test` names. They MAP correctly (verified against patch/log names)
  but are ambiguous. Fix in proposed copy: expand every cell to full name
  (e.g. `BatteryStructural.RlsResetGateBlockedWhenUnconnected`).

### F8. H-D5/H-D7 c2 pairs are NOT strict decision flips (should-fix wording)
- `MCDC_MATRIX.md:11-12` (H-D5: HST-01 vs HST-04) and `:23-24` (H-D7: HST-05 vs HST-08):
  the "c2" side (HST-04/HST-08, request==state) makes L75 `(_requested_state != _state)` FALSE,
  so L77/L83 is NEVER EVALUATED on that side. Only c1 differs under evaluation; c2's effect
  is shown end-to-end (armed-vs-quiescent via the entry guard), not as a same-decision T↔F flip.
- Short-circuit/masking note in matrix (`MCDC_MATRIX.md:14,26`) correctly identifies the
  unreachable arcs (L77 b=false-with-a=true; L83 equivalents — confirmed by final `.info`:
  `hysteresis.cpp BRDA:77,0,3,0` and `BRDA:83,0,1,0` + `83,0,3,0` never taken) but overclaims
  "c2 independent (only c2 differs)". Fix: relabel as "c2 demonstrated at system level via
  entry guard (L75); strict L77/L83 flip is masked by construction — see BRDA zeros above".
  Recomputation: H-D5 c1 pair (HST-02 vs HST-03) VALID (only `_state` differs, both evaluate L77);
  H-D7 c1 pair (HST-06 vs HST-07) VALID. c2 pairs WEAK — keep tests, fix claim.

### F9. B-D4 c2 pair is single-test sequential, not two tests (should-fix wording)
- `MCDC_MATRIX.md:34`: c2 `(!initialized)` uses "BST-07 (mid vs final)" — two calls inside ONE test
  (pre-init fires 15.0, post-init holds 15.0). Logically valid (only `!init` differs, others held:
  conn T, IRinit T, n>0 T) but violates the "which TWO TESTS" expectation.
- BRDA evidence: `battery.cpp BRDA:130` all 8 sub-arcs taken (counts 50/46/26/24/24/2/22/2) — gate
  fully exercised. Fix: keep the test, but workbook col D must read
  "BST-07 call1 vs BST-07 call2 (same test, sequential — state flip)" and report §5 must disclose it.

### F10. B-D10 c2 independence NOT demonstrated — report claim contradicted by .info (must-fix)
- `MCDC_MATRIX.md:66-68` admits the NaN-current probe "was removed" and claims
  "c2 independence is evidenced by code inspection + the skip path being exercised
  (BRDA L375 arcs both taken per coverage)".
- Final `.info` refutes "both taken": `battery.cpp BRDA:375,0,3,0` — one sub-arc never taken.
  So `armed=T + finite=F` was NEVER executed by submitted tests. Condition `finite(current)`
  shows only T (BST-19) at the decision; F appears only via `armed=F` (BST-18, masked).
- Fix: change matrix + report §5/§7 to honest wording: "B-D10 c2 (finite=F with armed=T)
  NOT demonstrated by submitted tests; independence incomplete; BRDA 375,0,3 = 0.
  Needed test: armed + NaN current probe asserting average holds (add, do not claim)."
  Do NOT invent a passing probe.

### F11. B-D9 c1 gap justification is concrete — ACCEPT, with scope note (no fix, keep wording)
- `MCDC_MATRIX.md:58` justifies `!finite(avg)=T` unreachable via public API: sole writer
  L380/L383 guarded by B-D10's `PX4_ISFINITE(current)`; `reset()` installs finite
  `BAT_AVRG_CURRENT`. This cites exact lines + mechanism — ACCEPTS as "technically concrete"
  (not bare "hardware dependent"). Consequence: condition c1 never T, decision still shows
  T (via c2/c3) + F (all-false) — MC/DC incomplete for c1 by construction. Keep as stated.
  Note supporting BRDA: `371,0,1,0` + `371,0,2,0` never taken (consistent with c1-never-T).

### F12. B-D8 / B-D11 / P-D3 pairs VALID (no fix; record for viva)
- B-D8 (L327-328 `&&`): BST-10 (T,T→fault) vs BST-12 (F,T→clean) isolates n_cells;
  BST-10 vs BST-11 (T,F exact 17.64→clean) isolates strict `>`. VALID. Duplicate base row is harmless.
- B-D11 (L377-378 `!fw || (fresh && LEVEL)`): BST-19 (T,—,—→update) vs BST-20 (F,F,T→hold)
  VALID under masking (c2/c3 masked when c1=T); BST-20 vs BST-27 isolates freshness; BST-27 vs
  BST-29 isolates LEVEL. VALID — keep short-circuit explanation.
- P-D3 (PID.cpp:70 `&&`): PST-02 (F,F) → PST-03 (T,F) isolates dt; PST-03 → PST-04 (T,T) isolates
  history; PST-05 (F,T) re-confirms. Boundary PST-06 (dt==eps→F) vs PST-07 (2eps→T) VALID. P-D2
  single `isfinite` shown T (all integral tests) + F (PST-01 NaN-hold) — not MC/DC, correctly excluded.

### F13. Coverage-gap prose understates battery arcs (feeds TASK 2; should-fix pointer)
- `REPORT_DRAFT.md:81,83` correctly reports battery 170/248 arcs but explains ONLY line 72.
  `.info` shows 78 zero-hit BRDAs, of which ~70 are `e0` exception/template arcs
  (e.g. L54/57/58/59/61/62/69/76/79/82/85/88/90-96/158/167/191/224/235/241-247/254-264/356/380/383/401-431)
  plus source-relevant zeros at L371 (×2), L375 (×1). "Hardware dependent alone rejected" —
  TASK 2 must classify each. Fix: point §7 at the TASK-2 table; do not claim gaps "each named" until done.

### F14. Agent artifacts must NOT be submitted (packaging note)
- `sqe-a2/.omo/` + `sqe-a2/.codegraph -> /home/zexr/...` (broken symlink) are agent artifacts.
  `.gitignore` (373B) status UNVERIFIED — check it excludes them. Fix: ensure neither is copied
  into `submission_pkg/` (currently absent — good) and `.gitignore` covers them.

## 2. Source line anchors used (v1.17.0 @ d6f12ad, local `px4-autopilot/`)
- hysteresis: L48 `if (from_state)`, L59/L60 stamp logic, L75 entry guard,
  L77 `if (_state && !_requested_state)`, L79 `>=` expiry, L83 `else if (!_state && _requested_state)`, L85 `>=`.
- PID: L49 `if (update_integral)`, L61 `if (std::isfinite(integral_new))`, L70 `if ((dt > FLT_EPSILON) && std::isfinite(_last_feedback))`.
- battery: L61 index-clamp, L71-72 `PARAM_INVALID` gap, L119 recognition, L123 latch,
  L128 init gate, L130 4-cond RLS gate, L144 arming, L309-319 ladder, L327-328 spike fault,
  L370-371 3-cond `||` reset, L375 armed+finite gate, L377-378 FW gate, L379 dt branch,
  L389 capacity gate, L400-429 param init.

## 3. Prioritized must/should/nice + questions for Ali
**Must-fix before submission:**
1. F5 (Sheet2 source-ref column) + F6 (separate 2 gap rows) via `Workbook_proposed.xlsx` only.
2. F10 (B-D10 c2 honesty — retract "both taken", cite BRDA 375,0,3=0, request armed+NaN test).
3. F4 (lock failure count 2 vs 3 with author confirmation).
4. F3 (unify PID authorship to "AI-drafted, lead-verified fallback").
**Should-fix:** F1 (stale docx name), F2 (annotate superseded 8/10 entry), F7 (expand "..." refs),
F8/F9 (disclose guard-masked + single-test pairs), F13 (point §7 at TASK-2 table), F14 (gitignore check).
**Nice:** dedup B-D8 base row; fix H-D5/H-D7 c2 notation (`c2=F(req F)` → `req=F so c2=T`).
**Questions for Ali:** (1) 2 or 3 dev failures — describe the PID NaN one? (2) Add armed+NaN
B-D10 probe test or submit with known-incomplete c2? (3) Replace fallback PST file if Ashar delivers —
re-measure? (4) Who fills cover names/section in DOCX — verified? (5) Keep `Workbook.xlsx`
working name + prefixed submission copy (current) — confirm LMS expects prefixed only?
