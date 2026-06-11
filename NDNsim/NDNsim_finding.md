# NDNsim - Findings for Research Paper

Detection of **Interest Flooding (IFA)** and **Cache Pollution (CP)** attacks in
Named Data Networking using a **dual, unsupervised statistical detector** over
ns-3 / ndnSIM tracer telemetry. All numbers are produced by re-running the pipeline
(`notebooks/01_preprocessing.ipynb` → `02_eda.ipynb` → `03_updated_detection.ipynb`);
figures are referenced by path. Last regenerated: 2026-06-11.

---

## Abstract

Using ndnSIM we simulate three topologies (tree, dumbbell, DFN/mesh) under three
conditions each - normal, Interest Flooding, and Cache Pollution - for nine
600-second runs with attacks injected at t = 300 s. Because ndnSIM's tracers expose
only face-level packet/cache counters (and **no PIT state**), we engineer
ratio-based features and detect anomalies with a **per-node statistical detector
trained only on normal traffic** (no labels used in fitting). The central finding is
that **IFA and CP produce opposite ratio signatures** - IFA collapses
`satisfaction_ratio` and sustains, CP raises it and bursts - so a single detector
cancels out. We therefore use a **dual detector** (an IFA branch and a CP branch,
combined as `max`). It detects **every attacker node in every scenario (100%) within
0-1 s**, flags **zero benign consumers**, localizes both attacks to the attacker and
on-path routers, and holds a normal-traffic false-positive rate of **~4%**. This
closes the gap left by the miniNDN study, where CP was undetectable from aggregate
counters.

---

## 1. Experimental setup

### 1.1 Scenarios (9 = 3 topologies × 3 conditions)

ns-3 / ndnSIM C++ simulations, 600 s each; attacks start at t = 300 s (balanced
300 s normal / 300 s attack). Per-second ground-truth labels are written for
evaluation.

| Topology | Nodes | Normal | IFA attacker | CP attackers |
|---|---|---|---|---|
| Tree | 12 (6c, 4r, 2p) | yes | c1 | c1, c6 |
| DFN/mesh | 12 (6c, 4r, 2p) | yes | c1 | c1, c6 |
| Dumbbell | 10 (3c, 4r+bn, 2p) | yes | c1 | c1, c3 |

- **IFA:** attacker c1 floods `/evil` (100 interests/s across 4 streams) toward a
  custom **BlackholeProducer** that never replies → PIT pressure along the path.
- **CP:** attackers issue 100 distinct `/ndn/junk/*` names at 2 Hz each (200/s
  total) that *are* served and cached, evicting legitimate content from the
  100-entry LRU Content Store.
- Normal consumers use Zipf-Mandelbrot (CP runs) or CBR (IFA runs) at 10 Hz.

### 1.2 Telemetry and features

ns-3 tracers (`L3RateTracer`, `CsTracer`, `AppDelayTracer`) give per-node, per-second
counters: `InInterests`, `OutInterests`, `InData`, `InNacks`,
`InSatisfiedInterests`, `InTimedOutInterests`, `CacheHits`, `CacheMisses`, plus
per-packet delay/retx/hop. **No PIT state is available** - the core instrumentation
limit that motivated moving from miniNDN to ndnSIM.

Engineered ratio features: `cache_hit_ratio`, `satisfaction_ratio`, `timeout_ratio`,
`nack_ratio`, `interest_amp`, `data_ratio`, plus delay/retx/hop aggregates.

### 1.3 Detector - dual, unsupervised, statistical (NOT Isolation Forest)

A per-node, per-feature **z-score / control-chart** detector. There is **no sklearn,
no Isolation Forest, no `.fit()`** - and **no supervised training**. Baselines
(`mu`, `sigma`) are computed **only from the normal-scenario files**, first 200 s:

```python
train_normal = full[full["scenario"] == "normal"]   # normal runs only
baseline = mean/std per (topology, node, feature) over Time <= 200
```

Two branches, using **directional** z-scores (domain priors, not labels):

| Branch | Features (direction) | Persistence |
|---|---|---|
| **IFA** | `satisfaction_ratio`↓, `timeout_ratio`↑, `InInterests`↑ | sustained (consecutive steps) |
| **CP** | `cache_hit_ratio`↓, `InInterests`↑ | rolling window (handles bursty signal) |

`anomaly_score = max(ifa_score, cp_score)`; alarm threshold = 50. **Labels are used
only for evaluation** (detection rate, latency, FPR vs the t ≥ 301 attack window) -
never for fitting. The detector is thus fully unsupervised; the only attack-specific
knowledge is the *direction* of each feature shift, which is what lets one detector
separate IFA from CP.

---

## 2. Results

### 2.1 Detection rate and latency

Per-node detection vs ground truth (`processed/detection_latency.csv`):

| | Attacker nodes detected | Mean latency | Benign consumers falsely flagged |
|---|---|---|---|
| **IFA** | **3/3 runs, 100%** (c1 each) | ~1 s | **0** |
| **CP** | **3/3 runs, 100%** (both attackers each) | ~1 s | **0** |

Every attacker node - c1 in IFA; c1 + c6 (or c1 + c3 in dumbbell) in CP - is flagged
in every topology, within **0-1 s** of attack onset. **Not one benign consumer is
falsely flagged in any scenario.** Mean detection latency across all detected nodes
is **1.5 s**.

On-path **routers** are detected at **79% (IFA) / 89% (CP)**; the un-flagged routers
are precisely the **off-path** ones (e.g. tree node 4 = r4, which carries no attack
traffic) - i.e. the gaps are correct spatial behavior, not misses.
(`figures/detection/fig_pernode_detection_comparison.png`,
`figures/detection/fig_detection_latency.png`.)

### 2.2 Score separation and false-positive rate

Mean anomaly score, pre vs post attack onset (global detector,
`processed/full_scored_global.csv`):

| Scenario | pre-attack score | post-attack score |
|---|---|---|
| IFA | 1.7 | **53.3** |
| CP | 1.7 | **65.9** |

The score distribution is **bimodal** - mass near 0 (normal) or near 100 (attack),
almost nothing in the ambiguous 40-60 band - so the alarm threshold (50) separates
cleanly (`figures/detection/fig_score_dist_comparison.png`). **Normal-scenario
false-positive rate is 4.08%** at threshold 50. Global and per-topology detectors are
near-identical (the z-score baseline is inherently per-node, so pooling adds little).

### 2.3 The central finding - IFA and CP have opposite signatures

The EDA time series (`figures/eda/fig_timeseries_{topo}.png`) make the mechanism
explicit at t = 300 s:

| Feature | IFA | CP |
|---|---|---|
| `satisfaction_ratio` | **collapses** to ~0 | **rises** to ~0.6-1.0 |
| `timeout_ratio` | **spikes** and stays high | **oscillates** in bursts |
| `cache_hit_ratio` | ~unchanged | **drops** and oscillates |
| `InInterests` | jumps to ~80/s (sustained) | rises to ~40/s (bursty) |

IFA = a **clean, sustained** shift; CP = a **noisy, oscillating burst** (the 100 junk
streams completing in waves). Because `satisfaction_ratio` moves in **opposite
directions** for the two attacks, a single shared detector cancels them out - this is
exactly why the dual detector (separate branches, `max`-combined, different
persistence logic) is necessary, and why a plain unsupervised Isolation Forest on the
shared feature set fails on CP.

### 2.4 Feature discriminability (KS)

KS D-statistic, mean over nodes (`processed/ks_results.csv`):

| Feature | IFA | CP |
|---|---|---|
| InInterests / OutInterests | 0.26 | 0.34 |
| InData | 0.00 | 0.34 |
| satisfaction_ratio | 0.17 | 0.23 |
| timeout_ratio | 0.14 | 0.21 |
| cache_hit_ratio | 0.05 | 0.12 |

Interest-rate features lead for both attacks. **`InData` separates only for CP**
(D = 0.00 for IFA) - under IFA the blackhole producer returns no data, so `InData`
barely moves; under CP the junk content *is* served, so `InData` rises. The per-node
KS heatmaps (`fig_ks_heatmap_{topo}_{attack}.png`) localize the signal: for **CP**,
the **attacker consumer nodes (6 and 11)** show the strongest `cache_hit_ratio` and
`satisfaction_ratio` separation (D = 0.50) - the cache-pollution fingerprint - while
routers separate on interest rates.

---

## 3. Key findings (paper-ready)

1. **A single unsupervised dual detector covers both IFA and CP** across three
   topologies, detecting every attacker node within 0-1 s at a ~4% false-positive
   rate, with zero benign-consumer false alarms.
2. **IFA and CP have opposite ratio signatures** (satisfaction ↓/sustained vs
   ↑/bursty); detecting both requires *separate directional branches*, not one shared
   model. This is the core methodological contribution.
3. **CP is detectable from ns-3 face/cache counters** - specifically a
   `cache_hit_ratio` drop on attacker-adjacent nodes plus an `InInterests` rise -
   closing the gap where miniNDN's aggregate counters could not detect CP.
4. **Detection is path-localized**: attacker + on-path routers fire; off-path routers
   and benign consumers stay silent - useful for attribution/mitigation.
5. **The detector is fully unsupervised** (baseline from normal traffic only); labels
   serve evaluation only. Attack-type knowledge enters solely as feature *direction*.

---

## 4. Limitations

1. **No PIT visibility.** ns-3 tracers expose only face-level counters; the classic
   PIT-exhaustion signal for IFA is unavailable, so IFA detection relies on
   interest-rate and satisfaction/timeout ratios rather than PIT state.
2. **CP detection is burstier and slower-tailed.** Because CP produces oscillating
   bursts, some router detections lag (one DFN router at 9 s, one dumbbell router at
   6 s) and the CP branch needs a rolling window rather than simple persistence.
3. **Directional priors are hand-set.** The detector encodes which way each feature
   moves per attack. This is expert knowledge, not learned; a novel attack with a
   different signature would need a new branch.
4. **Threshold and baseline are global constants** (alarm = 50, baseline window =
   200 s). They are not adapted per node/topology; a more rigorous study would tune
   them with cross-validation on held-out normal data.
5. **Single attacker profile per attack, emulated traffic, balanced classes.**
   Multi-attacker, distributed, low-rate/stealth variants, and real traffic are not
   evaluated. The 50/50 normal/attack split is by design and not representative of
   base rates in deployment.
6. **`InData` carries no IFA signal** (blackhole producer) - useful to know it is
   CP-specific, but it means the feature set is partly attack-specialized.

---

## 5. Relationship to miniNDN (for the paper narrative)

| | miniNDN | NDNsim (this work) |
|---|---|---|
| Source | NFD internal state (PIT visible) | ns-3 tracers (no PIT) |
| Detector | unsupervised Isolation Forest | unsupervised dual z-score detector |
| IFA | detected (89-99% attacker) | detected (100% attacker, 0-1 s) |
| CP | **undetectable** in feature set | **detected** (100% attacker) |
| Why the change | - | tracer PIT limit + need to handle CP's opposite, bursty signature |

NDNsim is the more complete result: it adds CP detection and the opposite-signature
insight that a dual, direction-aware detector is required.

---

## 6. Figure catalog (with suggested captions)

`{topo}` ∈ {tree, dumbbell, dfn}; `{attack}` ∈ {ifa, cp}. Paths relative to
`NDNsim/`.

### 6.1 EDA - `figures/eda/`

**`fig_timeseries_{topo}.png`** - mean feature time series, normal vs IFA vs CP, with
the t = 300 s attack line. Shows the opposite signatures (satisfaction collapses for
IFA, rises for CP; timeout sustained vs bursty). *Caption: "Feature time series under
normal, IFA, and CP ({topo}). IFA and CP shift satisfaction and timeout ratios in
opposite directions, motivating a dual detector."*

**`fig_boxplots_{topo}.png`** - per-node feature distributions, normal vs attack.
*Caption: "Per-node feature distributions ({topo}); attacker-adjacent nodes show the
clearest separation."*

**`fig_correlation_{topo}.png`** - feature correlation matrices, normal vs attack.
*Caption: "Feature correlation under normal vs attack traffic ({topo})."*

**`fig_ks_heatmap_{topo}_{attack}.png`** - node × feature KS D-statistic for each
attack. For CP, attacker consumer nodes show the strongest cache_hit/satisfaction
separation; for IFA, routers separate on interest rates. *Caption: "KS D-statistic per
node and feature ({topo}, {attack}). CP localizes to attacker consumers via cache-hit
ratio; IFA localizes to on-path routers via interest rates."*

**`fig_feature_distributions.png`** - global feature histograms across all data.

### 6.2 Detection - `figures/detection/`

**`fig_score_timeseries.png`** - 6 panels (topology × attack); IFA/CP sub-scores and
combined score over time with attack line and alarm. IFA = clean sustained jump above
50; CP = bursty but clears the alarm. *Caption: "Dual-detector anomaly score over time.
IFA produces a sustained alarm; CP produces an oscillating-but-detected signal."*

**`fig_pernode_detection_comparison.png`** - 6 panels; per-node detection rate, global
vs per-topology, attacker and on-path routers near 100%, off-path/benign near 0.
*Caption: "Per-node detection rate. The detector localizes both attacks to the
attacker and on-path routers; benign consumers and off-path routers stay silent."*

**`fig_pernode_{topo}_{attack}_global.png`** - per-node detection bars for one
scenario (six files). *Caption: "Per-node detection ({topo}, {attack})."*

**`fig_detection_latency.png`** - detection latency per node, IFA vs CP, by role.
IFA ≤ 2 s; CP mostly ≤ 2 s with a few slower router tails. *Caption: "Detection
latency after attack onset. IFA is detected within ~1-2 s; CP is slightly slower and
burstier."*

**`fig_score_dist_comparison.png`** - anomaly-score histograms, global vs
per-topology, per topology. Bimodal at 0 and 100; clean split at the alarm.
*Caption: "Anomaly-score distributions. Scores are bimodal, separating normal from
attack at the alarm threshold; global and per-topology detectors agree."*

**`fig_timeseries_comparison.png`** - combined-score time series comparison across
scenarios.

### 6.3 Result tables - `processed/`

| File | Contents |
|---|---|
| `ks_results.csv` | KS D-statistic + p-value per (topology, node, attack, feature) |
| `detection_latency.csv` | per-node detected flag + latency, per topology/attack |
| `full_scored_global.csv` | every node-second scored by the global dual detector |
| `full_scored_pertopo.csv` | every node-second scored by per-topology detectors |
| `full_dataset.csv` | engineered features + labels for all 9 runs |
| `train_normal.csv` | normal-scenario rows used to build baselines |
| `{topo}_{scenario}.csv` | per-scenario engineered feature tables |

---

## 7. Reproducibility

```bash
cd NDNsim/notebooks
jupyter nbconvert --to notebook --execute --inplace 01_preprocessing.ipynb     # raw traces → processed/
jupyter nbconvert --to notebook --execute --inplace 02_eda.ipynb               # → figures/eda + ks_results
jupyter nbconvert --to notebook --execute --inplace 03_updated_detection.ipynb # dual detector → figures/detection
```

Notebooks use a relative project root
(`PROJECT_ROOT = cwd.parent if cwd.name == "notebooks" else cwd`) and read raw
tracer output from `ndnsim-research-main/ndn-research/results/` (kept for full
from-scratch reproducibility). Scenario definitions, parameters, and topology maps
are documented in `notebooks/scenarios_info.md`; the dual-detector rationale is in
`notebooks/problem_found_from_graph.txt`.
