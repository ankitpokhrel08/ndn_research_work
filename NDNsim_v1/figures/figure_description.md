# NDNsim v1 — figure descriptions (single-rate study)

Record of the figures in this folder. v1 is the **single-rate** NDNsim study (9 runs,
one high attacker rate) and the home of the deep per-node analysis. Full caption-ready
text for each figure is in [`../NDNsim_finding.md`](../NDNsim_finding.md) **§6 (Figure
catalog)**; this file is the quick index. For the **detection-floor** figures (the rate
sweep), see [`../../NDNsim_v4/figures/figure_description.md`](../../NDNsim_v4/figures/figure_description.md).

> Caveat carried by every v1 figure: the legitimate background used
> `NumberOfContents = 100`, which exhausts ~10 s in, so these are measured against a quiet
> network. They remain valid for the **localization / ablation / KS** story (structural
> properties of the attacks); the corrected, live-traffic detection numbers are v4.

---

## EDA (`eda/`)

| file | what it tells |
|---|---|
| `fig_timeseries_{tree,dumbbell,dfn}.png` | mean feature time series, normal vs IFA vs CP, with the t=300 attack line — shows where each attack perturbs the counters |
| `fig_boxplots_{topo}.png` | per-node feature distributions, normal vs attack — attacker-adjacent nodes separate most |
| `fig_correlation_{topo}.png` | feature correlation matrices, normal vs attack |
| `fig_ks_heatmap_{topo}_{ifa,cp}.png` | node × feature KS D-statistic per attack — localizes the signal (CP → attacker consumers via cache-hit ratio; IFA → on-path routers via interest rate) |
| `fig_feature_distributions.png` | global feature histograms across all data |

## Detection (`detection/`)

| file | what it tells |
|---|---|
| `fig_score_timeseries.png` | Isolation Forest anomaly score over time, attacker nodes vs the normal ±1σ band — attacker crosses the alarm within ~1 s of onset for both attacks |
| `fig_pernode_detection_comparison.png` | per-node detection rate (global vs per-topology) — attacker + on-path routers high, benign/off-path near 0 |
| `fig_detection_latency.png` | detection latency per node by role — attacker detected within ~1 s |
| `fig_score_dist_comparison.png` | anomaly-score histograms per scenario/topology — normal stays low, attack separates past the alarm |
| `fig_feature_ablation.png` | attacker detection + normal FPR for full / counts-only / ratios-only feature sets — the rate/count features carry the signal |
| `fig_spatial_localization.png` | mean score by node role + score vs hop distance — the score traces the attack's forwarding path (attacker > on-path routers > producers > off-path > benign) |

---

See `../NDNsim_finding.md` §6 for the full captions and the `processed/` result-table index.
