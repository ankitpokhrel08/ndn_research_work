# NDN Anomaly Detection - Interest Flooding & Cache Pollution

Unsupervised detection of **Interest Flooding (IFA)** and **Cache Pollution (CP)**
attacks in Named Data Networking (NDN), studied across two independent data-collection
tracks: an emulation track (**miniNDN**) and a simulation track (**NDNsim**).

Both tracks use the **same unsupervised Isolation Forest** detector. They are
complementary stages of the same investigation: miniNDN exposes the forwarder's
internal state (PIT visible) and detects IFA; NDNsim cannot see the PIT but, on
face-level tracer counters alone, the same Isolation Forest detects **both** IFA
and CP. The move from one track to the other was forced by a single instrumentation
limit (no PIT access in ndnSIM tracers), and the result is that one method covers
the whole study.

---

## The two tracks at a glance

| | **miniNDN** | **NDNsim** |
|---|---|---|
| Data source | NFD internal state (PIT **visible**) | ns-3 / ndnSIM tracers (**no PIT**) |
| Attacks | IFA only | IFA **and** CP |
| Labels | none (pure unsupervised) | ground-truth, used **for evaluation only** |
| Detector | unsupervised **Isolation Forest** | unsupervised **Isolation Forest** (same config) |
| IFA result | 89-99% attacker detection, <2% FPR | **100%** down to ~4 Hz; **floor ≈ 1.8 Hz** |
| CP result | not in feature set | graded floor; **≈ 44 Hz** (≈100% above 50 Hz) |
| Topologies | tree / dumbbell / DFN | tree / dumbbell / DFN |
| Findings doc | [`miniNDN/miniNDN_finding.md`](miniNDN/miniNDN_finding.md) | [`NDNsim_v4/Finalfindings.md`](NDNsim_v4/Finalfindings.md) (v1→v4 lineage) |

**Headline:** one unsupervised Isolation Forest detects both attacks across both
tracks. NDNsim closes the CP gap miniNDN could not - on face-level tracer counters
alone, with no PIT access - and does so with the **same** detector miniNDN uses, so
the whole study is unified under a single method.

> **Update — detection floor (rate sweep, final).** The single-rate **100% / 0%-FPR**
> numbers above were measured at *one* attacker rate, on data where the legitimate
> background traffic decayed to zero mid-run (a catalog-exhaustion bug). A corrected,
> catalog-fixed **rate sweep** (`NDNsim_v4/`, 297 runs) re-measures detection against
> *live* traffic and pins both attacks' floors: **CP has a graded floor at ≈44 Hz**
> (100% above 50 Hz, degrading smoothly to ~21% at 10 Hz) and **IFA has a floor at
> ≈1.8 Hz** (100% down to 4 Hz, ~60% at 1 Hz). The two floors sit an **order of
> magnitude apart**, and the FPR is a real **~1–2%** — not the trivial 0%. See
> [`NDNsim_v4/Finalfindings.md`](NDNsim_v4/Finalfindings.md).

---

## Repository structure

```
minor_project_refactored/
├── README.md                     ← this file
├── exp_setup_analysis.tex        ← paper draft (LaTeX)
│
├── miniNDN/                      ── EMULATION TRACK ─────────────────────────
│   ├── miniNDN_finding.md         paper-ready findings (IFA, Isolation Forest)
│   ├── REFACTOR_FINDINGS.md       data-quality + feature-design investigation
│   ├── Datacard/
│   │   ├── Logs/                  raw NFD *_metrics.jsonl (source of truth)
│   │   └── Datasets/              generated feature CSVs (reproduction data)
│   └── research_analysis/        canonical, self-contained analysis
│       ├── 01_eda.ipynb  02_global_model.ipynb  03_per_topology_models.ipynb
│       └── models/ figures/ results/ FINDINGS.md
│
└── NDNsim — SIMULATION TRACK (versioned v1 → v4; v4 is the one to cite) ────────

NDNsim_v1/                        ── v1: single-rate study + localization/ablation ──
├── NDNsim_finding.md             100% / 0%-FPR at one rate; dead background (see banner)
├── notebooks/
│   ├── 01_preprocessing.ipynb  02_eda.ipynb  03_updated_detection.ipynb
│   └── scenarios_info.md         (scenario / topology reference)
├── ndnsim-research-main/ndn-research/
│   ├── scenarios/               9 ndnSIM C++ sims (3 topo × normal/ifa/cp)
│   ├── extensions/              blackhole-producer.{hpp,cpp}
│   └── results/                 raw tracer output (gitignored — in the bundle)
├── processed/                    small result tables kept; full_*.csv gitignored
└── figures/                      eda/ + detection/

NDNsim_v2/  NEWDATA_FINDINGS.md   ── v2: first rate sweep, BROKEN (dead traffic) — kept as record
NDNsim_v3/  BESTDATA_FINDINGS.md  ── v3: catalog fix (NumberOfContents=300); floor shape, CP crossing interpolated

NDNsim_v4/                        ── v4: FINAL — both floors MEASURED ──────────
├── Finalfindings.md             CP ≈ 44 Hz, IFA ≈ 1.8 Hz (an order of magnitude apart)
├── notebooks/                    01_preprocessing.ipynb  02_detection_floor.ipynb
├── ndn-research/
│   ├── scenarios/               catalog-fixed sims (NumberOfContents=300)
│   └── results/                 297 runs / 1188 raw traces (gitignored — in the bundle)
├── processed/                    detection_floor.csv, attack_window_contrast.csv, ...
└── figures/                      diagnostic/ (traffic health) + detection/ (floor)

for_friend.md                    spec for the v4 top-up runs that pinned both attacks' floors
```

(v1 and v3 keep the `ndnsim-research-main/ndn-research/` wrapper; v2 and v4 use
`ndn-research/` directly. All four `results/` dirs and the heavy `full_sweep.csv` /
`full_*.csv` are gitignored — they ship in the data bundle.)

---

## Key findings (combined)

1. **One unsupervised Isolation Forest detects both attacks across both tracks** -
   89-99% (miniNDN, IFA) and 100% (NDNsim, IFA + CP) on the attacker node, at low to
   zero false-positive rates. No attack-specific or hand-tuned detector is needed.
2. **CP is detectable from face-level ns-3 counters.** It is not in miniNDN's
   feature set, but in NDNsim the same Isolation Forest flags every CP attacker
   (100%, 0% held-out FPR) from the interest-rate and cache features - closing the
   CP gap without PIT access.
3. **The result is robust** - stable across random seeds and across the global /
   per-topology choice; a feature ablation shows the rate/count features carry the
   attack signal (the attacker's large junk-interest load is a clear outlier).
4. **Detection is spatially localized** in both tracks - attacker + on-path routers
   light up, benign/off-path nodes stay silent - useful for attribution.
5. **PIT-based features are not the reliable IFA signal here.** In miniNDN the PIT
   barely moves at the polling rate; in NDNsim it is not observable at all. Detection
   leans on satisfaction/timeout/interest-rate features instead.
6. **Load-invariant ratio features matter for robustness** (miniNDN investigation):
   absolute rate features are load-dependent and brittle; ratios generalize.
7. **The attacks have detection floors an order of magnitude apart** (rate sweep,
   `NDNsim_v4/`). Against *live* background traffic, **CP degrades gracefully** as the
   attacker slows (100% above 50 Hz → ~21% at 10 Hz; **90% crossing ≈ 44 Hz**) because its
   only signal is volume; **IFA holds 100% down to 4 Hz and only breaks at ~1 Hz**
   (**floor ≈ 1.8 Hz**) because its signature is qualitative (timeouts/NACKs to the
   black-hole), not volumetric. The earlier "100% everywhere" reflected a dead-traffic
   artifact plus a floor below the sampled rates — not detector invincibility; the FPR is
   a real ~1–2%.

All training in both tracks is **unsupervised** - models/baselines are fit on normal
traffic only; where labels exist (NDNsim) they are used purely for evaluation.

---

## Reproducing the results

> **Data:** raw traces/logs and large feature CSVs are **not** in the repo (to keep it
> lean) - download the data bundle and follow [`DATA.md`](DATA.md) first. Figures and
> trained models are committed, so results are viewable/usable without the bundle; the
> bundle is only needed to re-run the pipeline end-to-end.

**miniNDN** (run in order; each notebook trains and saves its own models):
```bash
cd miniNDN/research_analysis
jupyter nbconvert --to notebook --execute --inplace 01_eda.ipynb
jupyter nbconvert --to notebook --execute --inplace 02_global_model.ipynb
jupyter nbconvert --to notebook --execute --inplace 03_per_topology_models.ipynb
```

**NDNsim** — the main single-rate analysis is **v1**; the detection-floor sweep is **v4**:
```bash
# v1: single-rate EDA + detection (localization, ablation, KS)
cd NDNsim_v1/notebooks
jupyter nbconvert --to notebook --execute --inplace 01_preprocessing.ipynb
jupyter nbconvert --to notebook --execute --inplace 02_eda.ipynb
jupyter nbconvert --to notebook --execute --inplace 03_updated_detection.ipynb

# v4: rate sweep → detection floor (both attacks)
cd ../../NDNsim_v4/notebooks
jupyter nbconvert --to notebook --execute --inplace 01_preprocessing.ipynb
jupyter nbconvert --to notebook --execute --inplace 02_detection_floor.ipynb
```
(v2 and v3 are earlier sweep stages — run the same way from their own `notebooks/` if you
want to reproduce the lineage.)

Both pipelines use relative project roots, read their raw data from within their own
track, and write models / figures / results back into the track. No paths need
editing.

---

## Where to read more

- **Per-track results & figure catalogs:** `miniNDN/miniNDN_finding.md`,
  `NDNsim_v1/NDNsim_finding.md` (single-rate + localization/ablation)
- **Detection floor (rate sweep, final — cite this):** `NDNsim_v4/Finalfindings.md`
- **Sweep lineage** (what each version did and lacked): `NDNsim_v2/NEWDATA_FINDINGS.md`
  (broken first sweep), `NDNsim_v3/BESTDATA_FINDINGS.md` (catalog fix, interpolated floor)
- **miniNDN engineering investigation** (data-quality, load-invariant features):
  `miniNDN/REFACTOR_FINDINGS.md`
- **NDNsim scenario reference:** `NDNsim_v1/notebooks/scenarios_info.md`
- **Paper draft:** `exp_setup_analysis.tex`
