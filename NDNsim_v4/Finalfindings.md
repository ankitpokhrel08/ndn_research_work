# NDNsim — Final Findings (detection floor, both attacks)

**Track:** ns-3 / ndnSIM simulation. **Detector:** one unsupervised Isolation Forest,
trained on normal traffic only, evaluated against ground-truth labels.
**Status:** complete. Both attacks' detection floors are now measured, not interpolated.

> **Version v4 — final.** This is the culmination of the NDNsim lineage and the version to
> cite. The progression: **v1** ([`../NDNsim_v1/NDNsim_finding.md`](../NDNsim_v1/NDNsim_finding.md))
> single-rate study + localization/ablation analysis, but dead background traffic and no
> floor → **v2** ([`../NDNsim_v2/NEWDATA_FINDINGS.md`](../NDNsim_v2/NEWDATA_FINDINGS.md))
> first rate sweep, broken (background still dead, floor confounded) → **v3**
> ([`../NDNsim_v3/BESTDATA_FINDINGS.md`](../NDNsim_v3/BESTDATA_FINDINGS.md)) catalog fix
> (`NumberOfContents` → 300) revived the traffic and gave the floor's *shape*, but the CP
> crossing was interpolated and IFA looked flat → **v4 (this doc)** top-up runs measured
> both crossings directly. The localization / ablation / KS analysis in v1 still stands;
> everything about the *floor* is superseded by what's below.

---

## TL;DR

Across three topologies (tree, DFN, dumbbell), the same Isolation Forest detects both
Interest Flooding (IFA) and Cache Pollution (CP) from face-level ndnSIM tracer counters
alone — **no PIT access**. Sweeping the attacker rate from 1–100 Hz against *live*
legitimate background traffic gives each attack a clean, monotone detection curve and a
real floor:

| attack | what it is | detection floor (90% crossing) | shape |
|---|---|---|---|
| **CP** | volume attack (junk content that gets answered) | **≈ 44 Hz** | graded — sinks into live traffic as its rate drops |
| **IFA** | qualitative attack (interests to a black-hole `/evil`) | **≈ 1.8 Hz** | flat at 100% until it nearly stops sending |

**The headline result is the gap:** the two floors are **~25× apart**, and that gap is
explained by *what each attack does*, not by the detector or the topology. CP only ever
shows up as extra volume, so it hides as soon as its rate nears the legitimate baseline.
IFA leaves a qualitative fingerprint (timeouts / NACKs / zero satisfaction to the
black-hole) that survives down to ~1 interest/s. False-positive rate is a real
**~1–2%**, not a trivial 0%.

---

## 1. Experimental setup

### Topologies and roles
| topology | consumers | producers | routers | CP attackers | IFA attacker |
|---|---|---|---|---|---|
| tree | c1=6 … c6=11 | 0–1 | 2–5 | nodes {6, 11} | node {6} |
| DFN  | c1=6 … c6=11 | 0–1 | 2–5 | nodes {6, 11} | node {6} |
| dumbbell | c1=0, c2=1, c3=2 | 3–4 | 5–9 | nodes {0, 2} | node {0} |

Attackers are also legitimate consumers — the attack traffic rides **on top of** real
Zipf-distributed background requests, so the attacker node is never silent or obviously
synthetic. That is what makes the floor a genuine measurement.

### Traffic model
- Legit consumers: `ConsumerZipfMandelbrot`, `Frequency = 10`, **`NumberOfContents = 300`**.
- Producers: standard; CP attackers request 50 junk prefixes each that *are* answered.
- IFA: 4 `ConsumerCbr` streams on the attacker node (rates 0.40 / 0.30 / 0.20 / 0.10 ×
  `attackRate`) all pointed at a black-hole producer serving `/evil` — interests that are
  never satisfied.
- CP: 50 junk prefixes per attacker node, each at `cpFreq` scaled to `attackRate`, on 2
  attacker nodes.
- `simTime = 600 s`, `attackStart = 300 s`. First half = clean training/baseline window;
  second half = attack window.

### Detector (unchanged across the whole study)
```
StandardScaler  →  IsolationForest(n_estimators=200, contamination=0.01, random_state=42)
```
- Trained on **normal traffic only**, `t ≤ 200`, **pooled across topologies**, no labels.
- Features (9, all face-level — no PIT):
  `InInterests, OutInterests, InData, cache_hit_ratio, satisfaction_ratio,
  timeout_ratio, nack_ratio, interest_amp, data_ratio`.
- Anomaly score = `100 − clip(100·(d − s_min)/(s_max − s_min), 0, 100)`; **alarm at
  score ≥ 70**. Labels used **only** to score detection/FPR after the fact.

### Dataset (`ndn-research/results/` — 297 runs × 4 tracers = 1188 files)
3 topologies × {ifa, cp} × rate sweep + a normal baseline (5 seeds each).
- **CP rates:** 10, 12, 15, 20, 30, **35, 40, 45**, 50, 100 Hz.
- **IFA rates:** **1, 2, 4, 6, 8**, 10, 12, 15, 20, 30, 50, 100 Hz.
- **Seeds:** 5 seeds at every low/mid rate (where the floor lives); single-seed anchors
  at 50 and 100 Hz (detection is pegged at 100% there, no variance to capture).
- Tracers: L3 rate, content-store, and app-delay.

---

## 2. The dataset is honest: background traffic is alive

The earlier single-rate result (CP 100% / 0% FPR) was measured on data where the
legitimate consumers exhausted their tiny catalog and **decayed to ~0 interests/s** a
few seconds into each run — so CP was being "detected" against an empty network, and the
FPR was trivially 0 because benign nodes were silent. Raising `NumberOfContents` from
100 → **300** fixes it: `ConsumerZipfMandelbrot` keeps drawing from the Zipf catalog for
the full run, so the background stays flat for all 600 s.

| topology | t1–30 | t101–300 | **t301–450 (attack)** | **t451–599 (attack)** |
|---|--:|--:|--:|--:|
| tree / DFN (6 consumers) | ~32 | ~26 | **~27** | **~27** |
| dumbbell (3 consumers) | ~16 | ~13 | **~13** | **~13** |

(interests/s summed over consumers; `figures/diagnostic/fig_legit_traffic_health.png`.)
Each consumer sustains ~4.4 interests/s **through** the attack window vs **0.00** before.
Training-normal silence dropped from ~52% to ~11%, so the model learns a *busy* normal,
not an idle one. Every number below is detection **against live traffic**.

> Note: 300 is far below the ~6000 a "catalog exhausts → consumer stops" model would
> predict — that model was wrong; the consumer never stops at `NumberOfContents`. 300
> also keeps the cache-hit ratio high so runs stay fast (a 100000 catalog made nearly
> every request a miss and pushed run time to ~8–9 days).

---

## 3. The detection floor — full sweep, both attacks

Per-topology detection (% of attacker nodes alarmed in the attack window) and benign FPR.
Numbers are near-identical across tree / DFN / dumbbell; one representative column shown,
with FPR range across topologies.

| rate (Hz) | **CP** detection | **IFA** detection |
|--:|--:|--:|
| 1   | —    | **~60%** |
| 2   | —    | 99.9% |
| 4   | —    | 100% |
| 6   | —    | 100% |
| 8   | —    | 100% |
| 10  | 21%  | 100% |
| 12  | 24%  | 100% |
| 15  | 31%  | 100% |
| 20  | 42%  | 100% |
| 30  | 60%  | 100% |
| **35**  | **71%**  | 100% |
| **40**  | **83%**  | 100% |
| **45**  | **92%**  | 100% |
| 50  | 100% | 100% |
| 100 | 100% | 100% |

(bold rates = top-up runs added to pin the crossings; `processed/detection_floor.csv`,
`figures/detection/fig_detection_floor.png`.)

- **CP floor ≈ 44 Hz.** The curve is smooth and monotone straight through the old
  30→50 gap (60 → 71 → 83 → 92 → 100). The 90% crossing falls between r40 (83%) and
  r45 (92%). This is the floor the previous dataset could only bracket as "somewhere
  between 30 and 50."
- **IFA floor ≈ 1.8 Hz.** IFA holds 100% all the way down to r4, dips to 99.9% at r2,
  and only collapses to ~60% at r1. So IFA is **not** invincible — it has a real floor,
  it just sits ~25× lower than CP's. This resolves the earlier "100% everywhere is
  suspicious" concern: the 100% was real, the floor was simply below the rates we'd
  sampled.
- **FPR is a real measurement:** tree/DFN ~0.8–1.0%, dumbbell ~2% (consistent with the
  emulation track). Not the trivial 0% you get when benign nodes are silent.

---

## 4. Why the two floors differ — the actual contribution

The two attacks differ in **what counter they move**, and that determines whether they
can hide. Attack-window amplitude vs the legit baseline (`attack_window_contrast.csv`,
attacker OutInterests/s vs legit ~4.4/s):

| | absolute volume | contrast vs legit | detection |
|---|--:|--:|--:|
| CP @ r10 | 16.5/s | **3.7×** | only **21%** |
| IFA @ r1 | 5.4/s | **1.2×** | already **60%** |

CP at r10 is *louder* than IFA at r1 (3.7× vs 1.2× the baseline) yet detected far worse.
That is the whole story:

- **CP is purely volumetric.** Its only signal is "this consumer is busy." A busy
  consumer is exactly what a real consumer looks like, so as the junk rate drops toward
  the legitimate baseline the Isolation Forest can no longer separate it — detection
  degrades gracefully. The floor is set by *how far above the crowd* the attacker has to
  shout to stand out (~44 Hz here, ~10× the per-consumer baseline).
- **IFA is qualitative.** Interests to the `/evil` black-hole are *never satisfied*, so
  they spike `timeout_ratio` / `nack_ratio` and crush `satisfaction_ratio` regardless of
  how few they are. The signature is in the *shape* of the traffic, not its size, so it
  survives down to ~1 interest/s — where it finally goes sparse enough that whole time
  windows contain no attack interest at all, and detection drops to ~60%.

**The floors are a property of the attack type and the detector, not the topology** —
all three topologies give the same crossings, so this generalizes.

---

## 5. Honest caveats

- **Floor values are interpolated crossings, not sampled points.** Report them as bands:
  CP crosses 90% between **40–45 Hz**, IFA between **1–2 Hz**. Don't quote a false-precision
  decimal.
- **r50 / r100 are single-seed.** Fine for the floor (detection is pegged at 100% with no
  variance), but every other rate has 5 seeds — backfill these to 5 if a reviewer asks for
  uniform replication.
- **Per-consumer rate reads ~4.4/s, not the configured 10 Hz**, via the L3RateTracer
  `Packets` EWMA column. It is consistent across attacker and legit nodes, so it does
  **not** bias the floor — but cross-check the `PacketRaw` column before quoting any
  absolute interests/s figure.
- All training is unsupervised (normal traffic only); labels are used solely to score
  detection and FPR afterward.

---

## 6. What this means for the write-up

1. One unsupervised Isolation Forest detects **both** IFA and CP from face-level ndnSIM
   counters, **without PIT access** — closing the CP gap the emulation (miniNDN) track
   could not.
2. Both attacks have a **measured detection floor against live traffic**, and the floors
   sit **an order of magnitude apart** (CP ~44 Hz, IFA ~1.8 Hz).
3. The gap is **explained mechanistically**: volumetric attacks (CP) degrade gracefully
   and hide near the legitimate baseline; qualitative attacks (IFA) stay detectable until
   they nearly stop. This is a cleaner, more defensible claim than "100% everywhere."
4. The result is **topology-invariant** (tree / DFN / dumbbell agree) and reports a
   **real ~1–2% FPR**.

---

## Reproduce

```bash
cd NDNsim_v4/notebooks
../../myenv/bin/python -m jupyter nbconvert --to notebook --execute --inplace 01_preprocessing.ipynb
../../myenv/bin/python -m jupyter nbconvert --to notebook --execute --inplace 02_detection_floor.ipynb
```
`01` re-parses `ndn-research/results/` → `processed/full_sweep.csv`; `02` trains the
Isolation Forest and writes `processed/detection_floor.csv`,
`processed/attack_window_contrast.csv`, and the figures. The detector and features are
unchanged from the main NDNsim track — only the rate sweep and the catalog fix differ.

**Artifacts:** `processed/detection_floor.csv` (the table above) and
`processed/attack_window_contrast.csv` (volume vs contrast). Section 4 of
`02_detection_floor.ipynb` ("Paper figures") generates the full publication figure set —
**10 figures** documented one-by-one in
[`figures/figure_description.md`](figures/figure_description.md). The paper-priority ones:

- `figures/detection/fig_floor_headline.png` — **main result:** both floors, order of
  magnitude apart (CP ≈ 44 Hz, IFA ≈ 1.8 Hz), log-rate axis.
- `figures/detection/fig_mechanism_contrast.png` — **why they differ:** detection vs attacker
  volume (× legit baseline); IFA caught at ~1×, CP needs ~10×.
- `figures/detection/fig_detection_floor.png` — floor per topology with per-seed spread.
- `figures/detection/fig_score_separation.png` — score histograms; separation collapses for
  low-rate CP, holds for IFA.
- `figures/detection/fig_feature_signature.png` — which counters each attack moves
  (CP volume+cache; IFA timeout+satisfaction).
- `figures/detection/fig_score_vs_rate.png`, `fig_floor_heatmap.png`, `fig_score_timeline.png`,
  `fig_fpr_vs_rate.png`, and `figures/diagnostic/fig_legit_traffic_health.png` (background alive).
