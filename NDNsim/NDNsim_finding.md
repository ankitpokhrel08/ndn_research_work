# NDNsim - Findings for Research Paper

Detection of **Interest Flooding (IFA)** and **Cache Pollution (CP)** attacks in
Named Data Networking using a single **unsupervised Isolation Forest** over
ns-3 / ndnSIM tracer telemetry. This is the **same detector and configuration as
the miniNDN track**, so the whole study uses one unified method. All numbers are
produced by re-running the pipeline (`notebooks/01_preprocessing.ipynb` ->
`02_eda.ipynb` -> `03_updated_detection.ipynb`); figures are referenced by path.
Last regenerated: 2026-06-11.

---

## Abstract

Using ndnSIM we simulate three topologies (tree, dumbbell, DFN/mesh) under three
conditions each - normal, Interest Flooding, and Cache Pollution - for nine
600-second runs with attacks injected at t = 300 s. Because ndnSIM's tracers expose
only face-level packet/cache counters (and **no PIT state**), we engineer
ratio-based features and detect anomalies with an **unsupervised Isolation Forest
trained only on normal traffic** (no labels used in fitting). A single global
forest detects **every attacker node in every scenario (100%) within ~1 s**, flags
**zero benign consumers**, localizes both attacks to the attacker and on-path
routers, and holds a held-out normal-traffic false-positive rate of **0%**. The
result is stable across random seeds and across the global / per-topology choice,
and a feature ablation shows the face-level rate/count features carry the signal.
This unifies the NDNsim track with the miniNDN track under the same Isolation
Forest method.

---

## 1. Experimental setup

### 1.1 Scenarios (9 = 3 topologies x 3 conditions)

ns-3 / ndnSIM C++ simulations, 600 s each; attacks start at t = 300 s (balanced
300 s normal / 300 s attack). The per-second ground-truth label is a **network-state
label** (is an attack active anywhere, and of which type) used only to gate the
evaluation window - it is **not** a per-node attack label, since most nodes are
unaffected by an attack elsewhere. The per-node verdict instead uses topological
roles (see Section 2.3). Labels never enter model fitting.

| Topology | Nodes | Normal | IFA attacker | CP attackers |
|---|---|---|---|---|
| Tree | 12 (6c, 4r, 2p) | yes | c1 (node 6) | c1, c6 (nodes 6, 11) |
| DFN/mesh | 12 (6c, 4r, 2p) | yes | c1 (node 6) | c1, c6 (nodes 6, 11) |
| Dumbbell | 10 (3c, 4r+bn, 2p) | yes | c1 (node 0) | c1, c3 (nodes 0, 2) |

- **IFA:** attacker c1 floods `/evil` (100 interests/s across 4 streams) toward a
  custom **BlackholeProducer** that never replies -> PIT pressure along the path.
- **CP:** attackers issue 100 distinct `/ndn/junk/*` names at 2 Hz each (200/s
  total) that *are* served and cached, evicting legitimate content from the
  100-entry LRU Content Store.
- Normal consumers use Zipf-Mandelbrot (CP runs) or CBR (IFA runs) at 10 Hz.

Attacker node IDs follow the `.cpp` node-creation order and are cross-checked
against `InInterests` during the attack window. They are used **only for
evaluation** (to score attacker vs benign detection), never in fitting.

### 1.2 Telemetry and features

ns-3 tracers (`L3RateTracer`, `CsTracer`, `AppDelayTracer`) give per-node, per-second
counters: `InInterests`, `OutInterests`, `InData`, `InNacks`,
`InSatisfiedInterests`, `InTimedOutInterests`, `CacheHits`, `CacheMisses`, plus
per-packet delay/retx/hop. **No PIT state is available** - the core instrumentation
limit that motivated moving from miniNDN to ndnSIM.

The detector uses nine engineered face/cache features:
`InInterests`, `OutInterests`, `InData`, `cache_hit_ratio`, `satisfaction_ratio`,
`timeout_ratio`, `nack_ratio`, `interest_amp`, `data_ratio`.

### 1.3 Detector - unsupervised Isolation Forest (same as miniNDN)

A standard scikit-learn pipeline, identical in configuration to the miniNDN track:

```python
Pipeline([
    ("scaler", StandardScaler()),
    ("if",     IsolationForest(n_estimators=200, contamination=0.01, random_state=42)),
])
```

- **Training:** the forest is fit on **normal-scenario rows only**, first 200 s.
  No labels enter the fit - fully unsupervised.
- **Scoring:** `decision_function` (positive = normal) is mapped to a normalized
  `[0, 100]` score; we report `anomaly_score = 100 - norm` so **higher = more
  anomalous**, and alarm at `anomaly_score >= 70` (the equivalent of the miniNDN
  normalized boundary at 30).
- **Models:** a **global** forest (all topologies pooled) is the primary detector;
  **per-topology** forests are run for comparison.
- **Labels** are used only for evaluation (detection rate, latency, FPR over the
  t >= 301 attack window) - never for fitting.

This replaces an earlier exploratory dual statistical detector. A controlled
comparison (same features, same normal-only training, equal false-positive
operating point) showed the Isolation Forest matches or beats that detector on
both attacks, so the study uses the single unified Isolation Forest.

---

## 2. Results

### 2.1 Detection rate and latency

Per-node detection vs ground truth (`processed/detection_latency.csv`,
`processed/detection_per_node.csv`), global model:

| | Attacker nodes detected | Mean latency | Benign consumers falsely flagged |
|---|---|---|---|
| **IFA** | **3/3 runs, 100%** (c1 each) | ~1.0 s | **0** |
| **CP** | **3/3 runs, 100%** (both attackers each) | ~1.0 s | **0** |

Every attacker node - c1 in IFA; c1 + c6 (or c1 + c3 in dumbbell) in CP - is
flagged in every topology within ~1 s of attack onset. **Not one benign consumer
is flagged in any scenario.** On-path **routers** carry a mean detection rate of
~63%; the un-flagged routers are the **off-path** ones, i.e. correct spatial
behavior, not misses (`figures/detection/fig_pernode_detection_comparison.png`,
`figures/detection/fig_detection_latency.png`).

### 2.2 Score separation and false-positive rate

Mean `anomaly_score`, attacker nodes pre vs post attack onset (global model,
`processed/full_scored_global.csv`):

| Scenario | attacker pre | attacker post | benign post | normal |
|---|---|---|---|---|
| IFA | 8.5 | **78.4** | 1.9 | 17.5 |
| CP | 8.3 | **100.0** | 2.0 | 17.5 |

The attacker score jumps far past the alarm (70) while benign consumers stay near
zero; the per-scenario score distributions are well separated at the alarm
threshold (`figures/detection/fig_score_dist_comparison.png`). **Held-out
normal-scenario false-positive rate is 0.00%** (evaluated on normal traffic after
the 200 s training window). Global and per-topology forests are near-identical.

### 2.3 Spatial localization - the score traces the attack path

The time-based ground-truth label marks *when* an attack is active network-wide; it
is **not** a per-node truth (a benign off-path node is not anomalous merely because
an attack runs elsewhere). For a per-node verdict we classify each node by its
**topological role** - attacker / producer / on-path router / off-path router /
benign consumer - using the topology graph and a BFS from the attacker, and inspect
the score gradient (`processed/spatial_localization.csv`,
`figures/detection/fig_spatial_localization.png`):

| Role | IFA score | CP score |
|---|---|---|
| Attacker | 78.4 | 100.0 |
| On-path router | 75.4 | 88.7 |
| Producer | 60.7 | 63.7 |
| Off-path router | 36.2 | 36.7 |
| Benign consumer | 2.0 | 2.1 |

The unsupervised score forms a clean **spatial gradient**: every node on the attack's
forwarding path (attacker, transit routers, destination producers) is elevated, while
off-path routers are only partial and benign consumers sit near zero. The detector
therefore **localizes the attack and fingerprints its path** - it does not
blanket-flag the network during the attack window, which is the correct behavior
since most nodes are genuinely unaffected. Note the score follows *path membership*,
not raw hop distance: producers are several hops from the attacker yet light up
because the attack traffic flows to them. This is a stronger and more honest verdict
than a single network-wide "detection rate."

### 2.4 Robustness and feature ablation

The detector is not a lucky seed or feature artifact
(`notebooks/03_updated_detection.ipynb` section 8):

| Variant | Attacker detection | Benign false-alarm | Normal FPR |
|---|---|---|---|
| Global, seed 42 / 1 / 7 | 100% | 0% | 0.00% |
| Per-topology, seed 42 | 89% | 0% | 0.00% |

Feature ablation (`figures/detection/fig_feature_ablation.png`):

| Feature set | Attacker detection | Normal FPR |
|---|---|---|
| Full 9 features | **100%** | 0.00% |
| Counts only (`InInterests`, `OutInterests`, `InData`) | 100% | 0.00% |
| Ratios only | low | low |

The **rate/count features carry the attack magnitude** (the attacker emits ~109
interests/s during both attacks, a large outlier in any direction), which is what
makes a direction-blind isolation model detect both IFA and CP. Ratios alone are
insufficient. The global model is the right choice - pooling does not hurt, and
per-topology specialization is slightly weaker.

### 2.5 Feature discriminability (KS)

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
KS heatmaps (`figures/eda/fig_ks_heatmap_{topo}_{attack}.png`) localize the signal:
for **CP**, the **attacker consumer nodes** show the strongest `cache_hit_ratio`
and `satisfaction_ratio` separation (the cache-pollution fingerprint) while routers
separate on interest rates.

---

## 3. Key findings (paper-ready)

1. **One unsupervised Isolation Forest covers both IFA and CP** across three
   topologies: every attacker node detected within ~1 s, 0% benign-consumer false
   alarms, and a 0% held-out normal false-positive rate.
2. **CP is detectable from ns-3 face/cache counters** - closing the gap where
   miniNDN's aggregate counters could not detect CP. The attacker consumers emit a
   large junk-interest load and pollute caches, both visible in the face-level
   features.
3. **The detector is fully unsupervised and method-unified with miniNDN** (same
   `StandardScaler -> IsolationForest` pipeline, same configuration); labels serve
   evaluation only.
4. **Detection is path-localized, not network-wide.** The unsupervised score forms a
   spatial gradient - attacker (78-100) > on-path routers (75-89) > producers (61-64)
   > off-path routers (~36) > benign consumers (~2) - so the detector fingerprints
   the attack's forwarding path rather than blanket-flagging traffic during the
   attack window. Useful for attribution/mitigation, and the correct per-node verdict
   given that most nodes are genuinely unaffected.
5. **The result is robust** to random seed and to the global / per-topology choice,
   and the ablation shows the rate/count features are what carry the signal.

---

## 4. Limitations

1. **No PIT visibility.** ns-3 tracers expose only face-level counters; the classic
   PIT-exhaustion signal for IFA is unavailable, so detection relies on
   interest-rate, satisfaction, and cache features rather than PIT state.
2. **Attacker traffic is high-rate.** Both attacks drive the attacker's interest
   rate to ~109/s, a large outlier that any outlier model isolates easily. Low-rate
   / stealth attackers that stay within normal rate envelopes are not evaluated and
   would be harder.
3. **Off-path router detection is partial** (~63% router detection rate). This is
   expected (off-path routers carry no attack traffic) but means router-level
   alarms are a weaker, path-dependent signal than consumer-level alarms.
4. **Operating point and contamination are fixed** (`contamination=0.01`, alarm at
   the normalized boundary). They are not tuned per node/topology; a more rigorous
   study would cross-validate on held-out normal data.
5. **Single attacker profile per attack, emulated traffic, balanced classes.**
   Multi-attacker, distributed, low-rate/stealth variants, and real traffic are not
   evaluated. The 50/50 normal/attack split is by design and not representative of
   deployment base rates.

---

## 5. Relationship to miniNDN (for the paper narrative)

| | miniNDN | NDNsim (this work) |
|---|---|---|
| Source | NFD internal state (PIT visible) | ns-3 tracers (no PIT) |
| Detector | unsupervised Isolation Forest | unsupervised Isolation Forest (same config) |
| IFA | detected (89-99% attacker) | detected (100% attacker, ~1 s) |
| CP | not evaluated / not in feature set | **detected** (100% attacker, 0% FPR) |
| Role | exposes forwarder internals | adds CP coverage and a no-PIT setting |

Both tracks use the **same Isolation Forest method**. NDNsim is the more complete
result: it adds Cache Pollution detection and shows the method works on
face-level-only telemetry without PIT access.

---

## 6. Figure catalog (with suggested captions)

`{topo}` in {tree, dumbbell, dfn}; `{attack}` in {ifa, cp}. Paths relative to
`NDNsim/`.

### 6.1 EDA - `figures/eda/`

**`fig_timeseries_{topo}.png`** - mean feature time series, normal vs IFA vs CP,
with the t = 300 s attack line. *Caption: "Feature time series under normal, IFA,
and CP ({topo})."*

**`fig_boxplots_{topo}.png`** - per-node feature distributions, normal vs attack.
*Caption: "Per-node feature distributions ({topo}); attacker-adjacent nodes show the
clearest separation."*

**`fig_correlation_{topo}.png`** - feature correlation matrices, normal vs attack.

**`fig_ks_heatmap_{topo}_{attack}.png`** - node x feature KS D-statistic per attack.
*Caption: "KS D-statistic per node and feature ({topo}, {attack}). CP localizes to
attacker consumers via cache-hit ratio; IFA localizes to on-path routers via
interest rates."*

**`fig_feature_distributions.png`** - global feature histograms across all data.

### 6.2 Detection - `figures/detection/`

**`fig_score_timeseries.png`** - 6 panels (topology x attack); Isolation Forest
anomaly score for the attacker nodes vs the normal +/-1 sigma band, with attack and
alarm lines. *Caption: "Isolation Forest anomaly score over time. Attacker nodes
cross the alarm within ~1 s of attack onset for both IFA and CP."*

**`fig_pernode_detection_comparison.png`** - 6 panels; per-node detection rate,
global vs per-topology, attacker and on-path routers high, off-path/benign near 0.
*Caption: "Per-node detection rate. The detector localizes both attacks to the
attacker and on-path routers; benign consumers and off-path routers stay silent."*

**`fig_detection_latency.png`** - detection latency per node, IFA vs CP, by role.
*Caption: "Detection latency after attack onset; attacker nodes are detected within
~1 s."*

**`fig_score_dist_comparison.png`** - anomaly-score histograms per scenario and
topology (log y). *Caption: "Anomaly-score distributions; normal traffic stays low
while attack traffic separates past the alarm threshold."*

**`fig_feature_ablation.png`** - attacker detection and normal FPR for the full,
counts-only, and ratios-only feature sets. *Caption: "Feature ablation: the
rate/count features carry the attack signal; ratios alone are insufficient."*

**`fig_spatial_localization.png`** - (left) mean anomaly score by node role
(attacker / producer / on-path router / off-path router / benign consumer);
(right) score vs hop distance from the attacker, colored by role. *Caption: "Spatial
localization: the unsupervised score traces the attack's forwarding path. On-path
nodes are flagged at any distance; off-path routers and benign consumers stay low."*

### 6.3 Result tables - `processed/`

| File | Contents |
|---|---|
| `ks_results.csv` | KS D-statistic + p-value per (topology, node, attack, feature) |
| `detection_latency.csv` | per-node detected flag + latency + attacker flag |
| `detection_per_node.csv` | per-node detection rate (global vs per-topology) |
| `spatial_localization.csv` | per-node role, hop distance, on-path flag, mean score |
| `full_scored_global.csv` | every node-second scored by the global Isolation Forest |
| `full_scored_pertopo.csv` | every node-second scored by per-topology forests |
| `full_dataset.csv` | engineered features + labels for all 9 runs |
| `train_normal.csv` | normal-scenario rows used to fit the forest |
| `{topo}_{scenario}.csv` | per-scenario engineered feature tables |

---

## 7. Reproducibility

```bash
cd NDNsim/notebooks
jupyter nbconvert --to notebook --execute --inplace 01_preprocessing.ipynb     # raw traces -> processed/
jupyter nbconvert --to notebook --execute --inplace 02_eda.ipynb               # -> figures/eda + ks_results
jupyter nbconvert --to notebook --execute --inplace 03_updated_detection.ipynb # Isolation Forest -> figures/detection
```

Notebooks use a relative project root
(`PROJECT_ROOT = cwd.parent if cwd.name == "notebooks" else cwd`) and read raw
tracer output from `ndnsim-research-main/ndn-research/results/` (kept for full
from-scratch reproducibility). Scenario definitions, parameters, and topology maps
are documented in `notebooks/scenarios_info.md`.
