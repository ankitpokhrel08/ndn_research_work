# New-data (rate-sweep) findings

**Goal of this dataset.** Regenerate every ndnSIM attack scenario across a range of
attacker rates, replicated over seeds, to measure the **detection floor** — the attacker
rate at which the unsupervised Isolation Forest stops catching the attack. That crossover
is the intended research contribution.

**Dataset.** `ndn-research/results/` — 45 scenarios × 5 seeds × 4 tracers = 900 files.
3 topologies (tree, dfn, dumbbell) × {ifa, cp} × rates {10,12,15,20,30,50,100} interests/s,
plus one normal baseline per topology. Naming: `{topo}-{attack}-r{rate}-run{seed}-{tracer}.{ext}`
and `{topo}-normal-run{seed}-{tracer}.{ext}`.

**Pipeline.** `notebooks/01_preprocessing.ipynb` (raw tracers → `processed/full_sweep.csv`,
carrying `rate` and `run` as extra dimensions) and `notebooks/02_detection_floor.ipynb`
(traffic-health diagnostic + Isolation Forest detection-floor curves). Same detector config
as the main track: `StandardScaler → IsolationForest(n_estimators=200, contamination=0.01,
random_state=42)`, trained on normal traffic only (t ≤ 200, no labels), pooled across
topologies. Alarm at anomaly score ≥ 70.

---

## What we asked for is here, and the generation was done correctly

- **45 × 5 × 4 = 900 files**, complete (no missing runs).
- **Seeds genuinely differ** — `run1` vs `run2` are no longer byte-identical. The
  `RngSeedManager` / `RngRun` fix worked.
- **Attacker rate scales linearly** with the `r` parameter (attacker `OutInterests` tracks
  the rate exactly: r100→200/s, r50→100/s, r10→20/s — the 2× is app-face + link-face).

Every point in the generation prompt was implemented. This is **not** a generation failure.

---

## The blocking problem: the legitimate background traffic is dead

The legitimate consumers emit only during the **first ~30 seconds**, then collapse to zero:

| topology | legit interests/s, t=1–10 | t=11–30 | t=31–60 | t=61–200 | t≥201 |
|---|--:|--:|--:|--:|--:|
| tree / dfn | ~30 | ~10 | ~2.5 | ~0.16 | **0** |
| dumbbell   | ~15 |  ~5 | ~1.2 | ~0.08 | **0** |

(see `figures/diagnostic/fig_legit_traffic_decay.png`). The attack window is t ≥ 301, so during
the **entire** evaluation period there is **zero** legitimate traffic — at every rate
(`processed/attack_window_contrast.csv`, `legit_out = 0.0` for all 42 cells).

**Root cause.** Every scenario uses
`ConsumerZipfMandelbrot` with `NumberOfContents = 100`. The catalog is exhausted at 10 Hz in
~10 s, after which the consumers stop requesting. The smoking gun: each consumer emits
≈100 interests total — exactly `NumberOfContents`. This defect is inherited from the
original scenarios; the sweep faithfully reproduced it.

Consequently the model's training "normal" window (t ≤ 200) is **~half silence** (only
52% of rows have any interest), so the detector effectively learns *normal ≈ idle network*.

---

## Detection-floor results (real, but confounded)

`figures/detection/fig_detection_floor.png`, `processed/detection_floor.csv`:

| attack | r10 | r12 | r15 | r20 | r30 | r50 | r100 |
|---|--:|--:|--:|--:|--:|--:|--:|
| **IFA** (all topologies) | 100 | 100 | 100 | 100 | 100 | 100 | 100 |
| **CP** (all topologies)  | ~66 | ~67 | ~90 | 100 | 100 | 100 | 100 |

- **IFA never bends** — no floor in 10–100 Hz. IFA points at a black-hole `/evil`, so its
  signature is *qualitative* (timeouts, NACKs, zero satisfaction) and fires against a silent
  network at any rate.
- **CP does bend** — a floor appears at **12–15 Hz**. CP requests real (junk) content that
  is answered, so its only signal is volume; at low rate it sinks into the idle/startup
  baseline.
- **Benign-consumer FPR = 0% everywhere — trivially.** The benign consumers are *silent*
  during the attack window, and silence reads as normal. This is not a precision measurement.

**Both curves are confounded by the dead background.** The CP "floor" is a floor against
*silence*, not against traffic. With healthy sustained background we expect the IFA curve to
start degrading (real traffic also produces some timeouts/NACKs) and the CP floor to move
(CP must out-shout real requests, not an empty cache). So these numbers cannot yet be
reported as the detection floor.

---

## What to send back to the friend

One-line fix in every scenario `.cpp` (raise the catalog so it never exhausts at 10 Hz over
600 s; needs ≥ 6000):

```cpp
consumerHelper.SetAttribute("NumberOfContents", StringValue("100000"));
```

Plus **one new generation-time sanity check** — the one whose absence let this through:

> In a normal run, a legit consumer must still emit ~10 interests/s at **t = 400**
> (rate-trace `OutInterests`, not zero).

The existing checks (file count, CsTracer nonzero, seeds differ) are all structural and pass
even with dead traffic — none of them looks at whether the background survives the run.

Once the background is alive, re-run these two notebooks unchanged: the low-rate end of both
curves will drop for an honest reason, the benign FPR becomes a real number, and the
crossover below the 90% line is the actual detection floor.
