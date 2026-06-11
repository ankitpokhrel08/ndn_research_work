# miniNDN - Refactor & Detection Findings

> Working research notes for the miniNDN side of the NDN anomaly-detection project.
> Captures what the data actually shows, the bugs found, the fixes applied, and the
> known limitations to carry into the paper. Last updated: 2026-06-11.

---

## 1. What miniNDN is

miniNDN emulates an NDN network and logs internal **NFD (NDN Forwarding Daemon)**
state per node as JSONL, polled every ~2-10 seconds. Each record exposes absolute
counters: `nPitEntries`, `nCsEntries`, `nInInterests`, `nOutInterests`, `nInData`,
`nInNacks`, `nOutNacks`, `nSatisfiedInterests`, `nUnsatisfiedInterests`, `nHits`,
`nMisses`.

From these we engineer **10 per-interval features** and train an unsupervised
**Isolation Forest** to flag anomalies (IFA = Interest Flooding Attack,
CP = Cache Pollution).

**Feature set (delta-based):**

| Feature | Type | Definition |
|---|---|---|
| `pit_size` | absolute | `nPitEntries` |
| `cs_size` | absolute | `nCsEntries` |
| `pit_growth_rate` | rate | `Δ nPitEntries / Δt` |
| `in_interests_rate` | rate | `Δ nInInterests / Δt` |
| `out_interests_rate` | rate | `Δ nOutInterests / Δt` |
| `in_data_rate` | rate | `Δ nInData / Δt` |
| `nack_rate` | rate | `Δ(nInNacks + nOutNacks) / Δt` |
| `cache_hit_ratio` | ratio | `Δ nHits / Δ(nHits + nMisses)` |
| `satisfaction_ratio` | ratio | `Δ nSatisfied / Δ(nSatisfied + nUnsatisfied)` |
| `unsatisfied_ratio` | ratio | `Δ nUnsatisfied / Δ(nSatisfied + nUnsatisfied)` |

Note: `unsatisfied_ratio = 1 − satisfaction_ratio` whenever there's activity, so it
is **redundant** (one independent feature fewer than it appears).

---

## 2. Key empirical findings

### 2.1 The PIT does NOT explode during IFA (in this data)

Standard NDN theory says IFA causes a sustained PIT explosion. **The logs do not
show this.** `nPitEntries` is statistically identical in normal vs attack:

| Scenario | nPitEntries mean | max |
|---|---|---|
| Normal (logs1) | 3.00 | 9 |
| Normal (dumbbell) | 3.00 | 9 |
| IFA (tree_ifa) | 3.06 | 9 |
| IFA (dumbbell_ifa) | 3.19 | 9 |
| IFA (new_ifa_1) | 3.09 | 10 |

**Why:** `nPitEntries` is an *instantaneous gauge* polled every few seconds. IFA
fills and drains the PIT (interests get NACKed/time out fast) *between* polls, so
the snapshot never catches the spike. **Consequence: `pit_size` and
`pit_growth_rate` are not discriminative here.** The PIT-explosion story is true in
principle but invisible at this sampling rate.

### 2.2 What actually separates IFA from normal

Measured per-interval (delta-based):

| Scenario | satisfaction_ratio | **nack_rate** | in_interests_rate |
|---|---|---|---|
| Normal (logs1) | 1.000 | **0.000** | 5.2 |
| Normal (dumbbell) | 1.000 | **0.001** | 6.4 |
| IFA (new_ifa_1) | 0.797 | **2.491** | 4.4 |
| IFA (tree_ifa) | 0.837 | **2.805** | 12.7 |
| IFA (dumbbell_ifa) | 0.597 | **7.148** | 12.2 |

**The real IFA discriminators are `nack_rate` (0 → 2.5-7.1) and
`satisfaction_ratio` (1.0 → 0.29-0.84).** Interest rate overlaps too much to rely on.

### 2.3 IFA is a spatial/partial signature, not network-wide

Within one attack capture, only a subset of nodes show the attack. Example
`new_ifa_1` per node:

| Node role | satisfaction | nack_rate | affected? |
|---|---|---|---|
| c1 (client) | 0.76 | 8.5 | yes |
| p1 (producer) | 0.29 | 4.3 | yes |
| r1, r2 (routers) | 0.29 | 8.5 | yes |
| c2-c6, p2, r3, r4 | ~0.99 | 0.0 | no |

Detection must therefore be evaluated **per affected node**, not as a flat
all-node average (which is diluted by the unaffected majority).

### 2.4 Cache Pollution (CP) is effectively undetectable in this feature set

| Scenario | cache_hit_ratio | satisfaction | nack |
|---|---|---|---|
| Normal | 0.002 | 1.000 | 0.000 |
| `logs_tree_cp` (CP, 16K lines) | 0.002 | 1.000 | 0.000 |

CP shows **no signature** in any available feature. Baseline cache hit ratio is
already ~0.002 (the workload rarely re-requests content), so CP has nothing to
degrade. This is a fundamental limitation of aggregate per-node NFD counters and is
**why CP detection was moved to the ndnSIM track**, where face-level cache/interest
counters make CP visible to the same Isolation Forest used here.

---

## 3. The production-model bug and the fix

### 3.1 Root cause: polluted training data (not clip bounds)

The original production model (consumed by `Monitor/`) trained on
`logs1 + logs_mesh1 + logs_dumbbell1`. In that data:

> **22.6% of "normal" samples have satisfaction_ratio < 0.9.**

Cause: a feature-engineering edge case - when an interval has no interest activity
(`d_total = 0`, e.g. an idle node), `np.where(d_total > 0, …, 0.0)` records
`satisfaction_ratio = 0.0`, i.e. logs an **idle interval as total failure**. The
long production runs contain many idle intervals; the model learned that *failure
looks normal* and could not flag attack nodes whose satisfaction collapses to ~0.3.

Comparison of training-data cleanliness:

| Normal dataset | satisfaction mean | std | % with sat < 0.9 |
|---|---|---|---|
| **Production** (logs1+mesh+dumbbell1) | 0.774 | 0.418 | **22.6%** |
| Research tree (tree_normal_1/2) | 0.999 | 0.036 | 0.2% |
| Research dumbbell | 0.999 | 0.023 | 0.1% |
| Research dfn | 0.999 | 0.031 | 0.3% |

A secondary note: Isolation Forest builds little split structure on features that
are ~constant in training, so even after un-clipping `nack_rate`, the model gains
little from it unless the *training distribution itself* is clean and well-formed.
The clip-bounds issue (degenerate `[0,0]` bounds collapsing `nack_rate`) was real
but **not** the dominant cause - fixing it alone moved detection only 8.5% → 11%.

### 3.2 Design lessons (these are the research takeaways)

1. **Training-data quality dominates.** Idle intervals must be dropped (or treated
   as satisfied), not logged as `satisfaction = 0.0`. Training on clean per-topology
   normal data (sat<0.9 only 0.2%) reproduces the research notebooks' 89-98% IFA
   detection; training on the polluted long runs does not.
2. **Delta-clipping at 0** (`max(0, Δ)`) avoids counter-wrap artifacts.
3. **Guarded rate clipping** - only clip a rate feature when its normal P1/P99 bound
   is non-degenerate (`hi > lo`); otherwise a feature that is ~0 in normal (e.g.
   `nack_rate`) gets collapsed to a constant and its attack spike is destroyed.
4. **Isolation Forest can't isolate on a near-constant feature.** A feature that is
   ~constant in training contributes no split structure, so extreme attack values on
   it don't extend the isolation path. The training distribution itself must carry
   variance on the discriminative features.

### 3.3 The load-invariant feature finding (key result)

The biggest result of the investigation. **Absolute rate features are load-dependent**
- the same topology under heavier *legitimate* traffic looks anomalous to a model
trained on lighter traffic. A 10-feature model trained on the clean research data
false-flagged **49.8%** of `logs_dumbbell1` normal traffic, purely because that run
carried ~2× the interest rate (identical satisfaction, just more volume).

Replacing absolute rates with **load-invariant ratios** fixes this:

| feature set | IFA detection | FPR dumbbell | FPR mesh |
|---|---|---|---|
| full 10 features | 59.7% | **45.3%** | 4.8% |
| **load-invariant (5)** | **63.1%** | **1.9%** | **1.3%** |

Load-invariant set: `satisfaction_ratio`, `cache_hit_ratio`, `nack_rate`,
`data_interest_ratio` (= in_data_rate / in_interests_rate),
`fwd_ratio` (= out_interests_rate / in_interests_rate).

It gives **higher** detection than the full set *and* removes the brittleness (45% →
2% FPR), because the ratios don't scale with traffic volume. Validation across all
attack captures: **63-74% network-level (affected-node) IFA detection at <2% FPR**,
on both research and production normal data.

> This is an independent, first-principles derivation of **why a ratio-based detector
> is the right design** - the same direction taken into the ndnSIM work. It also
> shifts detection from the attacker node to the affected routers/victims (good for
> "is an attack happening", less so for pinpointing the attacker).

One operational caveat surfaced: an **idle interval** (no interest activity) has
`fwd_ratio = 0` and looks anomalous to a model trained only on active traffic. Any
real-time scorer must gate out idle intervals (no traffic ⇒ not under attack, since
IFA/CP both generate heavy traffic).

### 3.4 The research model is the gold standard

Reproduced faithfully (train on clean research normal, threshold = 0.0, measure
attacker node):

| Attack set | c1 detection @0 | research FINDINGS.md claim |
|---|---|---|
| tree_ifa | 88.8% | 89.5% ✓ |
| dumbbell_ifa | 98.5% | 98.7% ✓ |
| dfn_ifa | 96.7% | 96.7% ✓ |
| new_ifa_1 | 78.0% | - |

The research notebooks (`02`, `03`) already use clean data and the `if hi > lo` clip
guard - **no bug there**. They are the canonical analysis, and (as of this refactor)
they **save their trained models** to `research_analysis/models/` for reproducibility.

---

## 4. Known limitations (carry into the paper)

- **CP is not detectable** with the current aggregate-counter feature set.
- **Producers (p1) are weakly detected** for IFA (~3%) even with the good model -
  their feature signature stays closest to normal. Routers/clients detect at 78-81%.
- **PIT features carry no signal** at this polling rate; the detector relies on
  `satisfaction_ratio` and `nack_rate`.
- **`unsatisfied_ratio` is redundant** with `satisfaction_ratio`.
- These limitations motivated moving CP detection to the ndnSIM track (face-level
  counters), where the same Isolation Forest detects both IFA and CP.

---

## 5. Current folder layout (post-cleanup)

```
miniNDN/
├── REFACTOR_FINDINGS.md         ← this file
├── README.md
├── Datacard/
│   ├── Logs/                    ← raw NFD *_metrics.jsonl (source of truth)
│   ├── Datasets/                ← generated feature CSVs (reproduction data)
│   ├── Logs_Generator/          ← synthetic log generator
│   └── MiniNDN_Scripts/         ← scenario scripts
└── research_analysis/           ← paper-grade analysis (canonical, self-contained)
    ├── 01_eda.ipynb
    ├── 02_global_model.ipynb     ← trains + saves global model
    ├── 03_per_topology_models.ipynb  ← trains + saves per-topology models
    ├── FINDINGS.md
    ├── models/                   ← models saved by the notebooks (reproducible)
    ├── figures/
    └── results/
```

The real-time `Monitor/` web dashboard, its `Models/`, and the `train_model.py`
trainer were **removed** - they were a separate demo product that did not feed the
research analysis. The `research_analysis/` notebooks are fully self-contained: they
read `Datacard/Logs/` directly, train their own models, and save them to
`research_analysis/models/`.

### Log inventory (`Datacard/Logs/`)

| Folder | Type | Use |
|---|---|---|
| tree_normal_1, tree_normal_2 | normal (clean) | **training** + research |
| dumbbell_normal, dfn_normal | normal (clean) | **training** + research |
| tree_ifa, dumbbell_ifa, dfn_ifa | IFA | research evaluation |
| new_ifa_1 | IFA | extra IFA capture |
| logs1, logs_mesh1, logs_dumbbell1 | normal (higher-load) | valid normal; drop idle intervals before use |
| logs_tree_cp, new_cp_1 | CP | no usable signal |
| logs_dumbbell_ifa | "IFA" | failed/mislabeled - looks identical to normal |

---

## 6. How to run

```bash
# Research / paper analysis (run in order). Each saves its models + figures + results.
#   research_analysis/01_eda.ipynb
#   research_analysis/02_global_model.ipynb        → research_analysis/models/global_ifa_model.joblib
#   research_analysis/03_per_topology_models.ipynb → research_analysis/models/pertopo_*.joblib
```

---

## 7. Refactor changelog (2026-06-11)

- Deleted superseded/redundant: `Notebooks/`, `Results/`, `ndn_pipeline.ipynb`,
  `ndn_test.ipynb`, `Datacard/generate_attack_datasets_fixed.ipynb`,
  `NDNsim/ndnsim-research-main/ndnSIM/`.
- Fixed path bug: research notebooks `PROJECT_ROOT / "Logs"` → `"Datacard" / "Logs"`.
- Added model-saving cells to notebooks `02`/`03` → `research_analysis/models/`
  (verified by executing both notebooks end-to-end).
- Removed the non-research real-time cluster: `Monitor/`, `Models/`,
  `Datacard/train_model.py`. Reproduction data (`Datacard/Datasets/`, `Logs/`) kept.
- Investigation findings (load-invariant features, training-data quality) retained in
  Section 3 as research takeaways.
