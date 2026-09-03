# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A **research repository** (not a deployable app) studying unsupervised detection of
**Interest Flooding (IFA)** and **Cache Pollution (CP)** attacks in Named Data
Networking (NDN). The deliverables are Jupyter notebooks, figures, trained models,
findings docs (`*_finding.md` / `*FINDINGS.md`), and two LaTeX papers
(`exp_setup_analysis.tex`, `final_report.tex`). There is no build/lint/test suite —
"running" the project means executing the notebook pipelines.

Read `README.md` and `DATA.md` first: `README.md` is the authoritative map of the two
tracks and the headline findings; `DATA.md` explains the repo-vs-bundle data split.

## Big-picture architecture

The whole study is **one unsupervised Isolation Forest** (`sklearn.ensemble`, fit on
normal traffic only) applied across **two independent data-collection tracks**. They
are complementary, not alternatives:

- **`miniNDN/`** — emulation track. Real NFD forwarders under Mini-NDN; exposes the
  forwarder's internal state (PIT visible). Detects **IFA**.
- **`NDNsim_v1..v4/`** — simulation track. ns-3 / ndnSIM; **no PIT access**, only
  face-level tracer counters. The same Isolation Forest detects **both IFA and CP**,
  which closes the gap miniNDN's feature set could not. The switch between tracks was
  forced by that one instrumentation limit (no PIT in ndnSIM tracers).

Labels exist only in the NDNsim track and are used **for evaluation only** — all
training in both tracks is unsupervised.

### NDNsim versioning (important)

`NDNsim_v1` → `NDNsim_v4` is a **lineage, not four separate experiments**. Cite v4.

- **v1** — single-rate study + localization/ablation. Its 100%/0%-FPR numbers were
  measured at one attacker rate on data with a catalog-exhaustion bug (background
  traffic decayed to zero mid-run), so they overstate detectability.
- **v2** — first rate sweep, **broken** (dead traffic). Kept only as a record.
- **v3** — catalog fix (`NumberOfContents=300`); floor shape, CP crossing interpolated.
- **v4** — **FINAL**, both detection floors measured on live traffic (297 runs):
  **CP ≈ 44 Hz** (graded), **IFA ≈ 1.8 Hz** — an order of magnitude apart; FPR is a
  real ~1–2%, not the trivial 0%. See `NDNsim_v4/Finalfindings.md`.

Directory-layout gotcha: **v1 and v3** nest sims under
`ndnsim-research-main/ndn-research/`; **v2 and v4** use `ndn-research/` directly.

### Data flow

```
NDNsim:  scenarios/*.cpp ──[ndnSIM/waf]──► results/*.txt ──[01_preprocessing]──► processed/*.csv ──[02,03]──► figures + models
miniNDN: Mini-NDN emulator ─────────────► Datacard/Logs/*_metrics.jsonl ──[research_analysis]──► models + figures
```

Raw traces/logs and large derived CSVs (`results/`, most of `processed/`,
`Datacard/Logs`, `Datacard/Datasets`) are **gitignored** and ship in a separate data
bundle (see `DATA.md` for the URL and restore paths). Figures and trained models
**are** committed, so results are viewable without the bundle; the bundle is only
needed to re-run end to end.

## Running the analysis (the "test/build" of this repo)

Notebooks use relative project roots and read/write within their own track — no paths
need editing. Run each track's notebooks **in numeric order**; each notebook trains and
saves its own models. Requires the data bundle restored (or committed CSVs where present).

```bash
# miniNDN (emulation track)
cd miniNDN/research_analysis
jupyter nbconvert --to notebook --execute --inplace 01_eda.ipynb
jupyter nbconvert --to notebook --execute --inplace 02_global_model.ipynb
jupyter nbconvert --to notebook --execute --inplace 03_per_topology_models.ipynb

# NDNsim single-rate analysis (v1)
cd NDNsim_v1/notebooks
jupyter nbconvert --to notebook --execute --inplace 01_preprocessing.ipynb
jupyter nbconvert --to notebook --execute --inplace 02_eda.ipynb
jupyter nbconvert --to notebook --execute --inplace 03_updated_detection.ipynb

# NDNsim detection-floor sweep (v4 — the one to cite)
cd NDNsim_v4/notebooks
jupyter nbconvert --to notebook --execute --inplace 01_preprocessing.ipynb
jupyter nbconvert --to notebook --execute --inplace 02_detection_floor.ipynb
```

Python is the gitignored `myenv/` virtualenv (Python 3.13). Notebook stack:
`scikit-learn`, `pandas`, `numpy`, `scipy`, `matplotlib`, `seaborn`, `jupyter`.

## Regenerating the raw data (rarely needed)

- **NDNsim sims** (`NDNsim_v*/…/ndn-research/`): an ndnSIM/ns-3 scenario-template
  project. Build with `./waf configure && ./waf`; drive runs with `run.py` /
  `run-sweep.sh` (sweeps topology × attack × rate × seed). Scenarios live in
  `scenarios/*.cpp` (`tree`/`dumbbell`/`dfn` × `normal`/`ifa`/`cp`); the CP/IFA sink is
  `extensions/blackhole-producer.{hpp,cpp}`. Requires a full ndnSIM-2.5 build — see
  `NDNsim_v4/ndn-research/README.md`. `run.py` is Python 2 (ndnSIM tooling).
- **miniNDN logs** cannot be regenerated from this repo — they are emulator captures
  (rawest source, bundle-only). The collector deployed into Mini-NDN is
  `miniNDN/Datacard/MiniNDN_Scripts/file_metrics_collector.py` (install at
  `apps/custom/`), driven by `mnndn.py`. `Datacard/Logs_Generator/generator.py`
  synthesizes production-shaped metrics for testing without a full emulation.

## Findings docs (read before changing conclusions)

The prose findings are the real output; keep numbers consistent with them.
`NDNsim_v4/Finalfindings.md` (final floors), `miniNDN/miniNDN_finding.md` (IFA),
`miniNDN/REFACTOR_FINDINGS.md` (data-quality + load-invariant ratio-feature
investigation — the key engineering lesson: absolute rate features are load-dependent
and brittle, ratios generalize). Superseded-stage records: `NDNsim_v2/NEWDATA_FINDINGS.md`,
`NDNsim_v3/BESTDATA_FINDINGS.md`.
