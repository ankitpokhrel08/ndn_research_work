# Best-data (catalog-fixed rate-sweep) findings

**Goal.** Regenerate every ndnSIM attack scenario across a range of attacker rates,
replicated over seeds, to measure the **detection floor** — the attacker rate at which the
unsupervised Isolation Forest stops catching the attack. That crossover is the research
contribution. The previous sweep could not measure it because the legitimate background
traffic died ~30 s into every run; this dataset fixes that.

**Dataset.** `ndnsim-research-main/ndn-research/results/` — 177 runs × 4 tracers = 708 files.
3 topologies (tree, dfn, dumbbell) × {ifa, cp} × rates {10,12,15,20,30,50,100} + a normal
baseline. Low rates {10,12,15,20,30} have **5 seeds** each (where the floor lives); high
anchors {50,100} have **1 seed** each (detection there is already ~100%). Naming unchanged:
`{topo}-{attack}-r{rate}-run{seed}-{tracer}.{ext}` and `{topo}-normal-run{seed}-{tracer}.{ext}`.

**Pipeline.** `notebooks/01_preprocessing.ipynb` → `processed/full_sweep.csv` (carries `rate`
and `run`); `notebooks/02_detection_floor.ipynb` → traffic-health diagnostic + Isolation
Forest floor curves. Detector unchanged from the main track: `StandardScaler →
IsolationForest(n_estimators=200, contamination=0.01, random_state=42)`, trained on normal
traffic only (t ≤ 200, no labels), pooled across topologies. Alarm at anomaly score ≥ 70.

---

## The fix worked: the background traffic is alive

`NumberOfContents` was raised from 100 → **300**. The legitimate consumers now emit a
sustained rate for the **entire** 600 s run instead of collapsing to zero:

| topology | consumers | t1–30 | t31–100 | t101–300 | t301–450 (attack) | t451–599 (attack) |
|---|--:|--:|--:|--:|--:|--:|
| tree / dfn | 6 | ~32 | ~26 | ~26 | **~27** | **~27** |
| dumbbell   | 3 | ~16 | ~13 | ~13 | **~13** | **~13** |

(interests/s summed over all consumers; see `figures/diagnostic/fig_legit_traffic_health.png`.)
Each consumer sustains ~4.4 interests/s through the attack window — compared to **0.00** in the
old data. Training-normal silence dropped from ~52% to **11%**, so the model now learns a
*busy* normal rather than an idle network.

> Note: 300 is well below the ~6000 a "catalog exhausts → consumer stops" model would
> predict. That model was wrong: `ConsumerZipfMandelbrot` keeps drawing from the Zipf catalog
> indefinitely and does not stop at `NumberOfContents`. 300 is plenty, and it keeps the cache
> hit ratio high so the simulation stays fast (a 100000 catalog made almost every request a
> miss and pushed run time to ~8–9 days).

---

## The detection floor (real, not confounded)

`figures/detection/fig_detection_floor.png`, `processed/detection_floor.csv`:

| attack | r10 | r12 | r15 | r20 | r30 | r50 | r100 |
|---|--:|--:|--:|--:|--:|--:|--:|
| **IFA** (all topologies) | 100 | 100 | 100 | 100 | 100 | 100 | 100 |
| **CP** (all topologies)  | ~21 | ~24 | ~31 | ~42 | ~60 | 100 | 100 |

- **IFA never bends — and that is a legitimate positive result.** IFA points at a black-hole
  `/evil`, so its signature is *qualitative* — timeouts, NACKs, zero satisfaction — and is
  detectable even when it is a small fraction of live traffic. No floor in 10–100 Hz; the
  detector is genuinely robust to low-rate IFA.
- **CP has a real, graded floor.** CP requests real (junk) content that is answered, so its
  only signal is **volume**. As the rate drops it sinks into the live legitimate baseline and
  the Isolation Forest stops flagging it. **The floor sits around 40–50 Hz** (the 90%
  crossing), degrading to ~21% at 10 Hz.
- **False-positive rate is a real measurement now:** held-out normal ≈ **2.2%**; benign
  consumers in the attack window ≈ **1–2%** — not the trivial 0% you get when benign nodes are
  silent.
- The numbers are near-identical across tree / dfn / dumbbell, so the floor is a property of
  the **attack type and detector**, not the topology.

### Why the floor moved vs the old (confounded) estimate

Old data put CP at ~66% at r10 — but that was detection *against silence*. Against live
traffic CP at r10 is **~21%**: an attacker hides far better inside a real crowd than inside an
empty network. This is exactly the effect the dead-traffic data masked, and the reason the
corrected sweep was worth running.

---

## Caveats / honest notes

- **High-rate anchors are single-seed.** r50 and r100 have one seed each; the 90% crossing is
  interpolated between r30 (5 seeds, ~60%) and r50 (1 seed, 100%). Backfilling r50/r100 to 5
  seeds would tighten the crossing — worth doing if the exact floor value is reported.
- **Per-consumer rate is ~4.4/s, not the configured 10 Hz** (using the L3RateTracer `Packets`
  EWMA column). This is consistent and applies to attacker and legit nodes alike, so it does
  not bias the floor; but if an absolute interests/s number is quoted, cross-check against the
  `PacketRaw` column.
