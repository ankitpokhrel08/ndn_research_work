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
| IFA result | 89-99% attacker detection, <2% FPR | **100% attacker** detection, ~1 s latency |
| CP result | not in feature set | **100% attacker** detection, **0% held-out FPR** |
| Topologies | tree / dumbbell / DFN | tree / dumbbell / DFN |
| Findings doc | [`miniNDN/miniNDN_finding.md`](miniNDN/miniNDN_finding.md) | [`NDNsim/NDNsim_finding.md`](NDNsim/NDNsim_finding.md) |

**Headline:** one unsupervised Isolation Forest detects both attacks across both
tracks. NDNsim closes the CP gap miniNDN could not - on face-level tracer counters
alone, with no PIT access - and does so with the **same** detector miniNDN uses, so
the whole study is unified under a single method.

> **Update — detection floor (rate sweep).** The single-rate **100% / 0%-FPR** numbers
> above were measured at *one* attacker rate, on data where the legitimate background
> traffic decayed to zero mid-run (a catalog-exhaustion bug). A corrected, catalog-fixed
> **rate sweep** (`NDNsim_best_data/`) re-measures detection against *live* traffic and
> finds the real picture: **CP has a genuine detection floor** (≈100% above ~50 Hz,
> degrading to ~21% at 10 Hz; the 90% crossing sits near 40–50 Hz) and a **real ~2%
> false-positive rate** — not the trivial 0%. **IFA stays at 100% down to 10 Hz** and is
> being probed at lower rates to find its floor (or explain why it has none). See
> [`NDNsim_best_data/BESTDATA_FINDINGS.md`](NDNsim_best_data/BESTDATA_FINDINGS.md).

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
└── NDNsim/                       ── SIMULATION TRACK ────────────────────────
    ├── NDNsim_finding.md          paper-ready findings (IFA + CP, Isolation Forest)
    ├── notebooks/
    │   ├── 01_preprocessing.ipynb  02_eda.ipynb  03_updated_detection.ipynb
    │   └── scenarios_info.md  (scenario/topology reference)
    ├── ndnsim-research-main/ndn-research/
    │   ├── scenarios/            9 ndnSIM C++ sims (3 topo × normal/ifa/cp)
    │   ├── extensions/           blackhole-producer.{hpp,cpp}
    │   └── results/              raw tracer output (kept for full reproducibility)
    ├── processed/                derived CSVs (working reproduction data)
    └── figures/                  eda/ + detection/

NDNsim_best_data/                ── RATE SWEEP / DETECTION FLOOR ──────────────
├── BESTDATA_FINDINGS.md          catalog fix + real detection-floor results
├── notebooks/                    01_preprocessing.ipynb  02_detection_floor.ipynb
├── ndnsim-research-main/ndn-research/
│   ├── scenarios/               catalog-fixed sims (NumberOfContents=300)
│   └── results/                 raw swept traces (gitignored — in the bundle)
├── processed/                    detection_floor.csv, attack_window_contrast.csv, ...
└── figures/                      diagnostic/ (traffic health) + detection/ (floor)

NDNsim_newdata/                  (superseded first sweep — dead-traffic bug; kept as record)
for_friend.md                    spec for the top-up runs that pin both attacks' floors
```

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
7. **The attacks have different detection floors** (rate sweep, `NDNsim_best_data/`).
   Against *live* background traffic, **CP degrades gracefully** as the attacker slows
   (≈100% above ~50 Hz → ~21% at 10 Hz; floor near 40–50 Hz) because its only signal is
   volume; **IFA does not bend at all in 10–100 Hz** because its signature is qualitative
   (timeouts/NACKs to the black-hole). The earlier "100% everywhere" reflected a
   dead-traffic artifact, not detector invincibility; the FPR is a real ~2%.

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

**NDNsim** (run in order; `01` re-parses the raw traces):
```bash
cd NDNsim/notebooks
jupyter nbconvert --to notebook --execute --inplace 01_preprocessing.ipynb
jupyter nbconvert --to notebook --execute --inplace 02_eda.ipynb
jupyter nbconvert --to notebook --execute --inplace 03_updated_detection.ipynb
```

Both pipelines use relative project roots, read their raw data from within their own
track, and write models / figures / results back into the track. No paths need
editing.

---

## Where to read more

- **Per-track results & figure catalogs:** `miniNDN/miniNDN_finding.md`,
  `NDNsim/NDNsim_finding.md`
- **Detection floor (rate sweep):** `NDNsim_best_data/BESTDATA_FINDINGS.md`
- **Pending top-up runs** (to pin both attacks' floors): `for_friend.md`
- **miniNDN engineering investigation** (data-quality, load-invariant features):
  `miniNDN/REFACTOR_FINDINGS.md`
- **NDNsim scenario reference:** `NDNsim/notebooks/scenarios_info.md`
- **Paper draft:** `exp_setup_analysis.tex`
