# miniNDN - Findings for Research Paper

Unsupervised detection of **Interest Flooding Attacks (IFA)** in Named Data
Networking using an Isolation Forest over per-node NFD telemetry. All numbers below
are produced by re-running the analysis pipeline (`research_analysis/01_eda.ipynb` →
`02_global_model.ipynb` → `03_per_topology_models.ipynb`); figures are referenced by
path. Last regenerated: 2026-06-11.

---

## Abstract

We collect per-node NDN Forwarding Daemon (NFD) telemetry from three emulated
topologies (tree, dumbbell, DFN/mesh) under normal traffic and under an Interest
Flooding Attack launched by a single consumer. From cumulative counters we engineer
ten per-interval features and train an unsupervised Isolation Forest on normal
traffic only. A single **global** model detects the attacking consumer with
**89.5% / 98.7% / 96.7%** detection on tree / dumbbell / DFN at a **<1.6%**
false-positive rate, and-crucially-produces a **spatial fingerprint**: the attacker
and the on-path routers (and the dumbbell bottleneck) are flagged at 88-99%, while
off-path routers, sibling consumers, and producers remain near-normal. The detector
keys on **traffic-rate and cache-occupancy features**; PIT-based features carry no
signal at this sampling rate. We report feature discriminability (KS tests +
permutation importance), a global-vs-per-topology comparison, and a candid set of
limitations (cache-pollution is undetectable; rate features are load-dependent).

---

## 1. Experimental setup

### 1.1 Topologies and data

| Topology | Nodes | Normal captures | Attack capture |
|---|---|---|---|
| Tree | 12 (c1-c6, p1-p2, r1-r4) | tree_normal_1, tree_normal_2 | tree_ifa |
| Dumbbell | 10 (c1-c3, p1-p2, r1-r4, bottleneck) | dumbbell_normal | dumbbell_ifa |
| DFN / mesh | 12 (c1-c6, p1-p2, r1-r4) | dfn_normal | dfn_ifa |

Node roles: **c** = consumer, **p** = producer, **r** = router, plus a shared
**bottleneck** in the dumbbell. In every attack capture the attacker is consumer
**c1**. Total normal training data ≈ **54,900** per-interval samples.

### 1.2 Features (10, delta-based, per polling interval)

`pit_size`, `pit_growth_rate`, `cs_size`, `cache_hit_ratio`, `satisfaction_ratio`,
`unsatisfied_ratio`, `in_interests_rate`, `out_interests_rate`, `in_data_rate`,
`nack_rate`. Rate features are `Δcounter/Δt`; ratio features are delta-based
(`Δnum/Δden`) to avoid drift from monotonically growing cumulative counters.

### 1.3 Model and evaluation

- **Isolation Forest** (200 trees, contamination 0.01) inside a `StandardScaler`
  pipeline, trained on **normal traffic only** (unsupervised / one-class).
- Raw `decision_function` scores are mapped to a **0-100% anomaly scale**; the
  decision boundary (raw score 0) corresponds to **30%**. Detection is reported at
  two operating points: **@30% (strict)** and **@50% (lenient/operational)**.
- **Detection rate** = fraction of a node's attack-time intervals flagged.
  **FPR** = fraction of that node's normal intervals flagged.
- Two model regimes: a single **global** model (all topologies pooled) and
  **per-topology** models.

---

## 2. Results

### 2.1 Headline detection (attacker consumer c1)

| Topology | Global det@30 | Per-topo det@30 | Global FPR@30 |
|---|---|---|---|
| Tree | **89.5%** | 91.2% | 0.59% |
| Dumbbell | **98.7%** | 98.3% | 0.86% |
| DFN | **96.7%** | 96.7% | 1.62% |

The attacker is reliably detected across all three topologies at a low false-positive
rate. Global and per-topology models are comparable **for the attacker node**; they
diverge for routers (§2.4).

### 2.2 Spatial attack fingerprint (the key contribution)

Per-node global-model detection (`results/global_detection_per_node.csv`):

| Topology | Detected @30% (≥88%) | Detected only @50% | Stays normal (<5% @30) |
|---|---|---|---|
| Tree | c1*, r1, r2 | p1 (90.5% @50) | c2-c6, r3, r4, p2 |
| Dumbbell | c1*, r1, r2, r3, **bottleneck** | p1 (64.8% @50) | c2, c3, r4, p2 |
| DFN | c1*, r1, r2 | p1 (43.3% @50) | c2-c6, r3, r4, p2 |

\* attacker. The model lights up **the attacker and the routers on the attack path**
(and the dumbbell bottleneck, which carries all cross-traffic), while **off-path
routers (r3/r4 in tree and DFN), sibling consumers, and the second producer remain
near-normal**. This selective, path-localized response means the detector does not
merely say "an attack is happening" - its per-node output **localizes the attack
path**, which is directly useful for mitigation/attribution.

> Figure: `figures/global/fig_pernode_detection_dumbbell.png` shows c1, r1, r2, r3,
> and the bottleneck at ~99% while sibling consumers and producers sit near 0.

The **producer (p1)** is a special case: it is flagged only at the lenient 50%
threshold (43-90% across topologies). Under IFA the producer is reached by a reduced,
distorted request stream rather than a flood, so its deviation is milder.

### 2.3 Score separation

`figures/global/fig_score_dist.png` - normal vs IFA anomaly-score densities per
topology. Separation is **cleanest on tree** (normal mass at 70-90%, IFA mass pushed
below the 30-50% band) and **dumbbell** (a distinct IFA mode near 10-30%). On **DFN**
the two distributions overlap more (the mesh disperses attack traffic over more
paths), explaining its slightly higher FPR. This visual matches the numeric FPR
ordering tree < dumbbell < DFN.

### 2.4 Global vs per-topology models

`figures/per_topo/fig_detection_grid.png` (six panels, det@30 and det@50 per
topology). Key result: **the global model detects on-path routers far better at the
strict threshold.** Example (dumbbell, det@30):

| Node | Global | Per-topology |
|---|---|---|
| r1 | 98.7% | 20.9% |
| r2 | 98.7% | 16.2% |
| bottleneck | 98.7% | 15.5% |

The per-topology models only catch the routers at the lenient 50% threshold, whereas
the global model catches them at 30%. Pooling topologies gives the model a broader
notion of "normal," sharpening router-level sensitivity without raising FPR. **The
global model is therefore preferred.** (Per-topology models reach 100% on the
attacker at det@50 but also flag every node at that threshold - i.e. they lose
spatial selectivity.)

### 2.5 Feature discriminability

**Kolmogorov-Smirnov D-statistic** (normal vs attack, mean over nodes;
`results/ks_test_results.csv`, `figures/eda/fig_ks_heatmap_*.png`):

| Feature | Mean D | Max D |
|---|---|---|
| cs_size | **0.725** | 0.935 |
| in_interests_rate | **0.684** | 0.996 |
| out_interests_rate | **0.639** | 0.991 |
| in_data_rate | 0.453 | 0.992 |
| satisfaction_ratio | 0.394 | 0.987 |
| unsatisfied_ratio | 0.394 | 0.987 |
| nack_rate | 0.391 | 0.985 |
| cache_hit_ratio | 0.241 | 0.992 |
| pit_size | 0.093 | 0.571 |
| pit_growth_rate | **0.028** | 0.119 |

**Permutation feature importance** for the global model
(`figures/global/fig_feature_importance.png`) agrees on the top group:
`out_interests_rate`, `in_interests_rate`, `in_data_rate`, `cache_hit_ratio`
dominate; `pit_growth_rate` and `nack_rate` contribute almost nothing.

Two important reads of the KS heatmap (`fig_ks_heatmap_tree.png`):
- **`cs_size` and the interest-rate features are the workhorses** - D ≈ 0.85-0.99 on
  routers and producers. The attack manifests as a **traffic-rate surge and cache
  growth on the attack path.**
- **`nack_rate` and `satisfaction_ratio` are highly discriminative on the attacker
  and on-path nodes (D ≈ 0.88) but contribute little marginal importance** - they are
  *redundant* with the interest-rate features (when interests flood and fail, nacks
  and interest-rates move together), so the trees split on the rate features first.

---

## 3. Key findings (paper-ready)

1. **A single unsupervised global Isolation Forest detects IFA across three
   topologies** at 89.5% / 98.7% / 96.7% (tree/dumbbell/DFN) on the attacker, with
   <1.6% FPR, using normal traffic only.
2. **The detector produces a spatial fingerprint of the attack path** - attacker +
   on-path routers (+ bottleneck) flagged at 88-99%; off-path nodes stay
   near-normal. This localizes, not just detects.
3. **The attack signature is a traffic-rate surge plus cache growth.** `cs_size` and
   interest-rate features are the strongest discriminators (KS D up to 0.99);
   permutation importance confirms them.
4. **PIT-based features are uninformative at this sampling rate** (`pit_size` mean
   D = 0.09, `pit_growth_rate` = 0.03). The PIT fills and drains between polls, so an
   instantaneous gauge never captures the classic "PIT explosion."
5. **The global model beats per-topology models** at router-level detection (98.7%
   vs ~17-21% at the strict threshold), because pooled normal traffic gives a richer
   baseline.
6. **Producers are detectable only at a lenient threshold** - they see a distorted,
   reduced request stream rather than a flood.

---

## 4. Limitations

1. **Cache Pollution (CP) is undetectable in this feature set.** CP captures
   (`logs_tree_cp`, 16 k samples) are statistically identical to normal in every
   feature (cache_hit_ratio 0.002 vs 0.002). The baseline hit ratio is already ~0, so
   CP has nothing to degrade in the aggregate per-node counters. Detecting CP would
   require per-content-class or per-prefix cache telemetry, which NFD's aggregate
   counters do not expose.
2. **Rate features are load-dependent → the model is not load-robust.** `cs_size`
   and the interest-rate features that drive detection scale with traffic *volume*.
   A model trained on one normal load false-flags legitimately heavier traffic: the
   10-feature model trained on the clean captures flagged **49.8%** of a 2×-heavier
   (but perfectly normal) capture. A deployable detector should use **load-invariant
   ratio features** (`data_interest_ratio = in_data/in_interests`,
   `fwd_ratio = out_interests/in_interests`), which we show reach **63-74%**
   network-level detection at **<2% FPR robust across loads** (see
   `REFACTOR_FINDINGS.md` §3.3). This independently motivates a **ratio-based
   detector** - the direction taken into the ndnSIM work.
3. **PIT instrumentation is sampling-limited.** Because `nPitEntries` is an
   instantaneous gauge polled every few seconds, the most theoretically diagnostic
   IFA signal (PIT exhaustion) is invisible here. Higher-frequency or
   integrated/peak PIT instrumentation would be needed to exploit it.
4. **Producer detection is weak at the operational threshold** (det@30 ≈ 2-3%);
   producers are only caught when the threshold is relaxed to 50%.
5. **`unsatisfied_ratio` is redundant** (= 1 − `satisfaction_ratio` whenever there is
   activity); it adds no independent information.
6. **Idle intervals are a scoring hazard for any real-time deployment.** Intervals
   with no interest activity must be gated out (no traffic ⇒ not under attack); a
   model trained on active intervals will otherwise false-positive on idle ones.
7. **Single attacker, single attack family, emulated data.** Results are for one
   IFA consumer per run on emulated miniNDN traffic; multi-attacker, distributed, or
   low-rate stealth IFA, and real-world traffic, are not evaluated here.

---

## 5. Implications and future work

- Move to **load-invariant ratio features** for deployment robustness (validated;
  the ndnSIM direction).
- Add **per-prefix / per-content cache telemetry** to make CP detectable.
- Add **higher-frequency or peak PIT instrumentation** to recover the PIT signal.
- Extend to **multi-attacker and low-rate stealth IFA**, and to a supervised or
  hybrid (rule + IF) detector that can also fingerprint the attacker node, since the
  ratio-based unsupervised detector localizes the *victims/path* rather than the
  attacker.

---

## 6. Figure catalog (with suggested captions)

`{topo}` ∈ {tree, dumbbell, dfn}. Paths are relative to
`miniNDN/research_analysis/`. For each figure: **what it shows** and a
*suggested caption*.

### 6.1 Exploratory data analysis - `figures/eda/`

**`fig_boxplots_{topo}.png`** - 2×5 grid, one box-plot per feature; for every node,
normal (blue) vs attack (red) distributions side by side. Shows the attacker c1 with
a large `in_interests_rate` jump (≈15→32 on tree), `satisfaction_ratio` dropping and
`nack_rate` rising; `pit_size`/`pit_growth_rate` essentially unchanged.
*Caption: "Per-node feature distributions under normal vs IFA traffic ({topo}
topology). The attacker (c1) and on-path nodes show elevated interest rates and
collapsed satisfaction; PIT features are unchanged."*

**`fig_correlation_{topo}.png`** - two 10×10 feature-correlation matrices, normal vs
IFA. Under attack the interest-rate / data-rate block stays strongly correlated and
new structure appears (nack_rate becomes strongly anti-correlated with
satisfaction_ratio). *Caption: "Feature correlation under normal vs IFA traffic
({topo}). The attack reorganizes correlations, coupling nack rate and satisfaction
to the interest-rate block."*

**`fig_timeseries_{topo}.png`** - per-feature time series for representative nodes
(attacker, benign consumer, router) with the attack window highlighted. The attacker
column shows a clear step change in satisfaction/nack at attack onset; the benign
consumer is flat. *Caption: "Feature time series across the attack window ({topo}).
The attacker (c1) exhibits an abrupt satisfaction drop and nack spike at onset; a
benign consumer (c2) is unaffected."*

**`fig_ks_heatmap_{topo}.png`** - node × feature heatmap of the KS D-statistic
(normal vs attack), attacker row outlined in red. Highlights `cs_size`,
`in/out_interests_rate`, `in_data_rate` as D≈0.85-0.99 on the attack path and the
PIT columns as ≈0. *Caption: "Kolmogorov-Smirnov D-statistic per node and feature
({topo}). Traffic-rate and cache-size features separate attack from normal on the
attack path; PIT features do not."*

### 6.2 Global model - `figures/global/`

**`fig_score_dist.png`** - three panels, normal vs IFA anomaly-score densities (tree/
dumbbell/dfn) with the 30% and 50% thresholds marked. Tree/dumbbell show clean
separation; DFN overlaps more. *Caption: "Global-model anomaly-score distributions,
normal vs IFA. Separation is cleanest on tree and dumbbell; the DFN mesh disperses
attack traffic, increasing overlap."*

**`fig_pernode_detection_{topo}.png`** - bar chart of per-node detection rate
(det@30 and det@50), colored by node role, attacker starred. Shows attacker +
on-path routers (+ bottleneck) near 100% and off-path nodes near 0.
*Caption: "Per-node detection rate ({topo}, global model). The attacker, on-path
routers, and the bottleneck are flagged at ~90-99%; off-path nodes remain
near-normal - a spatial fingerprint of the attack path."*

**`fig_zscore_heatmap_{topo}.png`** - node × feature heatmap of mean z-score (signed)
during the attack. Shows the *direction*: on c1/p1/r1/r2, `satisfaction_ratio`
z ≈ −26 to −59 and `nack_rate` z ≈ +5 to +10. *Caption: "Signed mean feature
z-scores under IFA ({topo}). Attack-path nodes show a steep negative satisfaction
shift and positive nack shift; off-path nodes stay near zero."*

**`fig_score_timeline_{topo}.png`** - anomaly score vs time for attacker, benign
consumer, and a router, with thresholds. The attacker sits well below 30% for the
whole attack window while the benign consumer hovers around 50%.
*Caption: "Anomaly-score timeline ({topo}). The attacker (c1) stays below the 30%
threshold throughout the attack; a benign consumer (c2) remains in the normal band."*

**`fig_threshold_sensitivity.png`** - detection-vs-threshold curves for c1 in each
topology plus the global FPR curve (ROC-like). c1 detection saturates (~90-99%) by a
~25% threshold while FPR stays <2% until ~60%. *Caption: "Threshold sensitivity:
attacker detection and false-positive rate vs decision threshold. The 30% operating
point yields ~90-99% detection at <2% FPR."*

**`fig_feature_importance.png`** - permutation importance for the global model on
normal vs attack data. Top features: `out/in_interests_rate`, `in_data_rate`,
`cache_hit_ratio`; `pit_growth_rate` and `nack_rate` near zero. *Caption:
"Permutation feature importance (global model). Detection is driven by interest-rate
and cache features; PIT and (redundant) nack features contribute little marginal
information."*

### 6.3 Per-topology vs global - `figures/per_topo/`

**`fig_detection_grid.png`** - 3×2 grid (rows = topologies, cols = det@30 strict /
det@50 operational), per-node bars comparing per-topology vs global models. Shows the
global model detecting routers at the strict threshold where per-topology models do
not. *Caption: "Per-node detection, per-topology vs global model, at strict (30%) and
operational (50%) thresholds. The global model detects on-path routers at the strict
threshold; per-topology models require the relaxed threshold."*

**`fig_score_dist_comparison.png`** - 6 panels, global (top row) vs per-topology
(bottom row) score distributions. Per-topology models pile IFA mass right at the 30%
line (thinner margin). *Caption: "Score distributions, global vs per-topology models.
The global model gives a wider normal/attack margin."*

**`fig_c1_score_comparison.png`** - attacker score-over-time, global vs per-topology,
per topology. Both keep c1 below threshold during the attack; curves nearly coincide.
*Caption: "Attacker (c1) score over time, global vs per-topology models. Both flag
the attacker; the global model generalizes without loss on the attacker node."*

**`fig_zscore_{topo}.png`** - per-topology-model z-score heatmaps (counterpart to the
global `fig_zscore_heatmap_{topo}.png`).

### 6.4 Result tables - `results/`

| File | Contents |
|---|---|
| `ks_test_results.csv` | KS D-statistic + p-value per (topology, node, feature) |
| `global_detection_per_node.csv` | per-node det@30/50, FPR@30/50, mean/std score (global) |
| `detection_summary.csv` | per-node detection global vs per-topology |
| `global_ifa_scored.csv` | every attack-time interval scored by the global model |
| `per_topo_scores.csv` | every attack-time interval scored by per-topology models |

---

## 7. Reproducibility

```bash
cd miniNDN/research_analysis
jupyter nbconvert --to notebook --execute --inplace 01_eda.ipynb
jupyter nbconvert --to notebook --execute --inplace 02_global_model.ipynb
jupyter nbconvert --to notebook --execute --inplace 03_per_topology_models.ipynb
```

Notebooks read raw logs from `../Datacard/Logs/`, train their own models, and write
`models/`, `figures/`, and `results/`. Trained models are saved to
`research_analysis/models/` (`global_ifa_model.joblib`, `pertopo_{tree,dumbbell,dfn}_model.joblib`).
See `REFACTOR_FINDINGS.md` for the data-quality and feature-design investigation
behind these results.
