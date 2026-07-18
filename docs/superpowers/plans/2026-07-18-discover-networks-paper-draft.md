# Discover Networks Paper Draft — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Produce a first-draft LaTeX manuscript of the NDN detection-floor paper in `paper_writing/`, ready to compile on Overleaf and submit to *Discover Networks*.

**Architecture:** Modular LaTeX — a Springer `sn-jnl` `main.tex` that `\input`s one focused `.tex` file per section from `sections/`, with `references.bib` and a `figures/` subset. Each section is drafted, structurally verified, and committed independently. Prose is adapted from `final_report.tex` (line ranges cited per task), re-shaped floor-first and to journal length.

**Tech Stack:** LaTeX (Springer Nature `sn-jnl`/`sn-article.cls`), BibTeX, optional `tectonic` for local compile. No app code.

## Global Constraints

- **Venue:** *Discover Networks* (Springer, OA, ISSN 3004-9792). Use official Springer Nature `sn-jnl` template, **numbered** citation style.
- **Title (verbatim):** "Detecting Network Anomalies in Named Data Networking Through Isolation Forest-Based Behavioral Analysis"
- **Authors (verbatim, order):** Ankit Pokhrel, Asmit Phuyal, Darshan Kumar Paudyal, Babu R. Dawadi (**corresponding**). Affiliation: Department of Electronics & Computer Engineering, Pulchowk Campus, Institute of Engineering, Tribhuvan University, Lalitpur, Nepal.
- **Framing:** floor-first narrative; detection floor is the headline result, Isolation Forest is the method.
- **Structure:** IMRaD modeled on `Citations/` (unstructured abstract + keywords; intro ends with contribution bullets + roadmap sentence; related work is a gap argument, not a list).
- **Cut:** real-time dashboard, 5-layer system architecture, sequence diagram → one sentence in Discussion.
- **Figure budget:** ~12, from `Figures/`.
- **Numbers are canonical** (quote exactly): IFA attacker detection @50% = 91.2 / 98.7 / 96.8 % (tree/dumbbell/DFN); emulation FPR <4.3%; KS: PIT pit_size D≈0.096, pit_growth D≈0.028, discriminating features D>0.9, p<1e-200; floors: CP ≈44 Hz (90% crossing), IFA ≈1.8 Hz; CP degrades 100%>50Hz → ~21% at 10Hz; IFA 100% to 4Hz, ~60% at 1Hz; simulation FPR ~1–2%; sweep = 297 runs (15 normal / 156 IFA / 126 CP); mechanism: IFA detected ~1× baseline, CP ~10×; feature shift CP ~+6σ volume/cache, IFA timeout ratio ~+3.7σ.
- **Floor phrasing:** quote crossings as bands in body text (CP 40–45 Hz, IFA 1–2 Hz); exact values only in headline/abstract. Note two highest sweep rates are single-seed.
- **Commit style:** short lowercase imperative subject, no AI attribution footer (per repo global rules).
- **Branch:** `paper/discover-networks` (already checked out).

---

## Task 1: Scaffold `paper_writing/` with Springer template

**Files:**
- Create: `paper_writing/main.tex`
- Create: `paper_writing/sn-jnl.cls` (+ any Springer support files, e.g. `sn-bibliography` bst)
- Create: `paper_writing/sections/` (empty stubs for all 10 section files)
- Create: `paper_writing/figures/.gitkeep`
- Create: `paper_writing/check.sh` (structural verifier)
- Create: `paper_writing/README.md` (how to compile on Overleaf / with tectonic)

**Interfaces:**
- Produces: `main.tex` that `\input{sections/<name>}` for: `abstract`, `introduction`, `related-work`, `method`, `emulation`, `simulation`, `mechanism`, `discussion`, `conclusion`, `declarations`; `\graphicspath{{figures/}}`; `\bibliography{references}`.
- Produces: `check.sh` used by every later task.

- [ ] **Step 1: Acquire the Springer template.** Download the `sn-jnl` LaTeX package. Try, in order:
  - `curl -L -o /tmp/sn.zip https://www.springernature.com/documents/sn-article-template.zip && unzip -o /tmp/sn.zip -d paper_writing/` (or the current template URL from the Discover Networks "submission guidelines" page).
  - If the download is unavailable, create a minimal `sn-jnl`-compatible fallback: `\documentclass[sn-mathphys-num]{article}`-style preamble is NOT valid; instead use a standard `\documentclass[11pt]{article}` preamble with `natbib` (numbered), `graphicx`, `booktabs`, `amsmath`, `subcaption`, `hyperref`, `geometry`, and leave a `% TODO-TEMPLATE: replace preamble with sn-jnl.cls before submission` comment. (This keeps the draft compilable; the real class is swapped in at submission.)

- [ ] **Step 2: Write `main.tex`** — preamble (per Step 1), title (verbatim from Global Constraints), author/affiliation block (verbatim), then the ten `\input{sections/...}` lines in the order listed above, `\bibliographystyle` (numbered, e.g. `sn-mathphys-num` if template present else `unsrtnat`), `\bibliography{references}`.

- [ ] **Step 3: Create empty section stubs** — each `sections/<name>.tex` contains only a `% <Name> — drafted in Task N` comment so `main.tex` inputs resolve.

- [ ] **Step 4: Write `check.sh`** (structural verifier that works without LaTeX):

```bash
#!/bin/bash
# paper_writing/check.sh — structural verification (no LaTeX engine needed)
cd "$(dirname "$0")"
fail=0
# 1. Placeholder scan
if grep -RInE 'TODO|TBD|FIXME|XXX|\bLorem\b|\?\?\?' sections/ main.tex 2>/dev/null | grep -v 'TODO-TEMPLATE'; then
  echo "FAIL: placeholder tokens found"; fail=1
fi
# 2. Every \ref/\eqref/\autoref target has a matching \label
for r in $(grep -RhoE '\\(ref|autoref|eqref)\{[^}]+\}' sections/ | sed -E 's/.*\{([^}]+)\}/\1/' | sort -u); do
  grep -RqF "\\label{$r}" sections/ || { echo "FAIL: undefined ref: $r"; fail=1; }
done
# 3. Every \cite key exists in references.bib
for c in $(grep -RhoE '\\cite[a-z]*\{[^}]+\}' sections/ | sed -E 's/\\cite[a-z]*\{//; s/\}//' | tr ',' '\n' | sort -u); do
  grep -qE "^@[a-zA-Z]+\{$c," references.bib 2>/dev/null || { echo "FAIL: undefined cite: $c"; fail=1; }
done
# 4. Every \includegraphics file exists in figures/
for f in $(grep -RhoE '\\includegraphics(\[[^]]*\])?\{[^}]+\}' sections/ | sed -E 's/.*\{([^}]+)\}/\1/' | sort -u); do
  ls figures/"$f"* >/dev/null 2>&1 || { echo "FAIL: missing figure: $f"; fail=1; }
done
[ $fail -eq 0 ] && echo "OK: structural check passed"
exit $fail
```

- [ ] **Step 5 (optional): Install tectonic for real compiles.** `brew install tectonic` (macOS). If it installs, later tasks may additionally run `tectonic main.tex`. If not, `check.sh` is the gate.

- [ ] **Step 6: Verify.** Run `bash paper_writing/check.sh`. Expected: `OK: structural check passed` (no sections cite/ref/include anything yet). Make `check.sh` executable: `chmod +x paper_writing/check.sh`.

- [ ] **Step 7: Commit.**
```bash
git add paper_writing/
git commit -m "scaffold discover networks paper (template, main.tex, check.sh)"
```

---

## Task 2: Build `references.bib` from the 9 citation PDFs

**Files:**
- Create: `paper_writing/references.bib`

**Interfaces:**
- Produces: BibTeX keys used by all later `\cite`s. **Canonical key list** (later tasks must use exactly these):
  `zhang2014ndn`, `shah2023security`, `xu_persistent_detection`, `liu2008isolation`, `xing2021isolation`, `liu2025xgboost`, `mastorakis2016ndnsim`, `caching_survey`, `ref_pb1` (rename once identified).

- [ ] **Step 1: Read each PDF's front matter to extract verified metadata.** Open page 1 (and page 2 if needed) of each file in `Citations/` with the Read tool and record title, authors, venue, year, volume/pages, DOI:
  - `anomaly_isolation_forest_ndn.pdf` → **verified:** Xing, Chen, Hou, Zhou, Dong, Zeng, Luo, Ma. "Isolation Forest-Based Mechanism to Defend against Interest Flooding Attacks in Named Data Networking." *IEEE Communications Magazine*, Mar 2021. → `xing2021isolation`
  - `futureinternet-17-00206-v2.pdf` → **verified:** Liu, Yu, Wu, Peng. "XGBoost-Based Detection of DDoS Attacks in Named Data Networking." *Future Internet* 2025, 17(5):206. DOI 10.3390/fi17050206. → `liu2025xgboost`
  - `Isolation_Forest.pdf` → Liu, Ting, Zhou. "Isolation Forest." *ICDM* 2008. → `liu2008isolation`
  - `Named_data_networking_NDN_project_vanjacobson.pdf` → Zhang et al. "Named Data Networking." *ACM SIGCOMM CCR* 44(3), 2014. → `zhang2014ndn`
  - `Security_and_Integrity_Attacks_in_Named_Data_Networking_A_Survey.pdf` → Shah et al. (verify authors/venue/year). → `shah2023security`
  - `Towards Persistent Detection of DDoS Attacks in NDN.pdf` → Xu et al. (verify). → `xu_persistent_detection`
  - `ndn-0028-1-ndnsim-v2.pdf` → Mastorakis, Afanasyev, Zhang. "ndnSIM 2: An updated NDN simulator for NS-3." NDN Tech Report NDN-0028, 2016. → `mastorakis2016ndnsim`
  - `Caching_on_Named_Data_Network_a_Survey_and_Future_.pdf` → caching survey (verify authors/venue/year). → `caching_survey`
  - `1060-802-1-PB1.pdf` → **identity unknown; open and verify before use.** → `ref_pb1` (rename to real key).

- [ ] **Step 2: Write `references.bib`** with one correct `@article`/`@inproceedings`/`@techreport` entry per file, using the keys above and the verified fields. No `note = {}` placeholders.

- [ ] **Step 3: Verify.** Run:
```bash
grep -c '^@' paper_writing/references.bib   # expect 9
```
Expected: `9`. Then eyeball that no entry has an empty `title`/`author`/`year`.

- [ ] **Step 4: Commit.**
```bash
git add paper_writing/references.bib
git commit -m "add references.bib from citation pdfs"
```

---

## Task 3: Assemble the figure subset

**Files:**
- Create: `paper_writing/figures/*.png` (copies of the ~12 selected figures)

**Interfaces:**
- Produces: figure files referenced by Tasks 4–11. **Selected set** (filenames as `\includegraphics` targets):
  `topologies`, `emu_score_dist`, `emu_pernode_tree`, `emu_pernode_dumbbell`, `emu_pernode_dfn`, `emu_ks_heatmap_tree`, `emu_feature_importance`, `fig_legit_traffic_health`, `fig_score_timeseries`, `fig_spatial_localization`, `fig_floor_headline`, `fig_detection_floor`, `fig_mechanism_contrast`, `fig_feature_signature`, `fig_floor_heatmap`, `fig_fpr_vs_rate`.

- [ ] **Step 1: Locate `topologies.png`.** Run:
```bash
find /Users/ankitpokhrel/Desktop/minor_project_refactored -iname 'topolog*' -not -path '*/myenv/*' 2>/dev/null
```
  If found, note the path. If **not found**, flag to the user (regeneration or a hand-made topology diagram is needed) and proceed with the rest; the Introduction figure will reference `topologies` and `check.sh` will report it missing until supplied.

- [ ] **Step 2: Copy the selected figures** from `Figures/` (and `topologies.png` from wherever Step 1 found it) into `paper_writing/figures/`:
```bash
cd /Users/ankitpokhrel/Desktop/minor_project_refactored
for f in emu_score_dist emu_pernode_tree emu_pernode_dumbbell emu_pernode_dfn \
         emu_ks_heatmap_tree emu_feature_importance fig_legit_traffic_health \
         fig_score_timeseries fig_spatial_localization fig_floor_headline \
         fig_detection_floor fig_mechanism_contrast fig_feature_signature \
         fig_floor_heatmap fig_fpr_vs_rate; do
  cp "Figures/$f.png" paper_writing/figures/
done
```

- [ ] **Step 3: Verify.** `ls paper_writing/figures/ | wc -l` — expect 15 (16 once `topologies.png` is added).

- [ ] **Step 4: Commit.**
```bash
git add paper_writing/figures/
git commit -m "add selected figures for paper"
```

---

## Task 4: Abstract + keywords

**Files:**
- Modify: `paper_writing/sections/abstract.tex`

**Content spec (write actual prose, ~200 words, unstructured, floor-first):**
- Adapt `final_report.tex:117-121`. One-paragraph arc: NDN + IFA/CP threat → single unsupervised Isolation Forest on two platforms (miniNDN real NFD, ndnSIM) with three topologies → emulation establishes IFA detection (91–99%, FPR <4.3%) and that PIT features are non-discriminating → **the central result:** an attacker-rate sweep (297 runs) locates each attack's *detection floor*, and the two floors are an order of magnitude apart (CP ≈44 Hz, IFA ≈1.8 Hz) at a realistic 1–2% FPR, explained by CP's volumetric vs IFA's qualitative signature → takeaway: the security-relevant limit is the mechanism-dependent detection floor.
- Follow with `\keywords{Named Data Networking; Interest Flooding Attack; Cache Pollution; Isolation Forest; anomaly detection; detection floor}` (or the sn-jnl keyword macro).

- [ ] **Step 1: Write the abstract + keywords** into `sections/abstract.tex` per the spec above.
- [ ] **Step 2: Verify.** `bash paper_writing/check.sh` → `OK`. Word count sanity: `wc -w paper_writing/sections/abstract.tex` (~180–230).
- [ ] **Step 3: Commit.** `git add -A && git commit -m "draft abstract and keywords"`

---

## Task 5: Introduction

**Files:**
- Modify: `paper_writing/sections/introduction.tex`

**Content spec (adapt `final_report.tex:167-210`; ~900–1200 words):**
- Para 1: NDN as content-centric future Internet; PIT/CS/FIB; the new attack surface.
- Para 2: IFA (PIT/timeout/NACK, qualitative signature) and CP (cache eviction, volumetric signature); why threshold detectors miss both.
- Para 3: **The gap** — prior work reports detection at a single high rate and never characterises the *detection floor* (the rate at which an attack becomes indistinguishable); the floor, not the peak, bounds stealth. Also: over-reliance on PIT features; single-platform validation.
- Para 4: This paper — one unsupervised Isolation Forest, two platforms, an attacker-rate sweep that measures both floors.
- **Contribution bullets** (itemize, mirror the `Citations/` style): (1) detection floor as the security metric; (2) two floors an order of magnitude apart, mechanistically explained; (3) one detector, cross-platform, topology-invariant; (4) PIT-feature myth correction.
- **Roadmap sentence:** "The remainder of this paper is organised as follows…" mapping Sections 2–8.
- Reference `\ref{fig:topologies}` and `\cite{zhang2014ndn}` at first NDN mention.

- [ ] **Step 1: Write introduction** into `sections/introduction.tex` (include `\label{fig:topologies}` on the topologies figure here or in Method — put it here).
- [ ] **Step 2: Verify.** `bash paper_writing/check.sh` → `OK` (topologies figure may report missing until Task 3 supplies it; acceptable, note it).
- [ ] **Step 3: Commit.** `git add -A && git commit -m "draft introduction"`

---

## Task 6: Related Work

**Files:**
- Modify: `paper_writing/sections/related-work.tex`

**Content spec (adapt `final_report.tex:216-236`; synthesise, don't list; ~700–1000 words):**
- Subsection flow: (a) NDN security & attack surveys — `\cite{zhang2014ndn,shah2023security,caching_survey}`; (b) IFA/CP detection & DDoS — `\cite{xu_persistent_detection,liu2025xgboost}`, noting supervised/labeled-data limits and single-rate evaluation; (c) Isolation Forest and its NDN application — `\cite{liu2008isolation,xing2021isolation}`, noting IFDM uses four PIT-derived features; (d) the simulator baseline `\cite{mastorakis2016ndnsim}`; (e) `ref_pb1` placed by topic once identified.
- Close with the four research gaps this paper closes (floor, PIT-myth, cross-platform, per-node localization) — the bridge into the contributions.

- [ ] **Step 1: Write related work** into `sections/related-work.tex`.
- [ ] **Step 2: Verify.** `bash paper_writing/check.sh` → `OK`.
- [ ] **Step 3: Commit.** `git add -A && git commit -m "draft related work"`

---

## Task 7: Method

**Files:**
- Modify: `paper_writing/sections/method.tex`

**Content spec (adapt `final_report.tex:275-397` + setup from `400-460`; ~1400–1800 words). Fold the experimental setup here.** Subsections:
1. **Platforms & topologies** — miniNDN (real NFD) vs ndnSIM; the three topologies (tree/dumbbell/DFN); `\ref{fig:topologies}`. (from report 403–427)
2. **Feature engineering** — 10 emulation features (Table, from report 316–336) and 9 ndnSIM face-level features (338); ratio-zero handling; clipping caveat (NACK/pit_growth not clipped to zero); load-invariant ratio note.
3. **Isolation Forest pipeline** — StandardScaler + IsolationForest (200 trees, contamination 0.01, fixed seed), trained on normal only; the IF anomaly-score equation (report 246–258).
4. **Auto-threshold & 0–100 normalization** — θ at decision_function 0; strict 30% / operational 50% (report 352–373).
5. **Detection-floor definition (NEW, formal):** define the detection floor as the smallest attacker rate at which pooled attacker-node detection ≥ 90%; state it is measured by the rate sweep.
6. **Evaluation metrics** — detection rate, FPR, KS D-statistic (report 262–269).
- Include Tables: 10-feature table, and software-environment table (report 464–483) — or move software table to an appendix; keep here for now.

- [ ] **Step 1: Write method** into `sections/method.tex` with the feature table and the two equations (`\label{eq:iforest}`, `\label{eq:floor}` or similar).
- [ ] **Step 2: Verify.** `bash paper_writing/check.sh` → `OK`.
- [ ] **Step 3: Commit.** `git add -A && git commit -m "draft method"`

---

## Task 8: Emulation study (miniNDN)

**Files:**
- Modify: `paper_writing/sections/emulation.tex`

**Content spec (adapt `final_report.tex:569-750`; ~1400–1800 words). Subsections:**
1. **Setup recap** — miniNDN traffic model, IFA attacker c1 (report 414–422); one or two sentences (detail is in Method).
2. **Detection & FPR** — Table (attacker detection @30/50%: 89.1/91.2, 98.5/98.7, 96.7/96.8) and FPR table (<4.3%); `\ref{fig:emu_score_dist}` (`emu_score_dist`).
3. **Per-node & spatial fingerprint** — `\ref{fig:emu_pernode}` (three subfigures: `emu_pernode_tree/dumbbell/dfn`); backbone routers 90–99%, on-path producer up to 90.5%, tree network-wide flood (51–58%).
4. **PIT-feature myth (KS)** — KS table (pit_size D≈0.096, pit_growth D≈0.028; discriminating features D>0.9); `\ref{fig:emu_ks}` (`emu_ks_heatmap_tree`) and `\ref{fig:emu_featimp}` (`emu_feature_importance`).
5. **CP negative result** — cache-hit already near zero → no CP footprint → **motivates the simulation study** (report 747–750). This is the bridge sentence into Task 9.

- [ ] **Step 1: Write emulation section** into `sections/emulation.tex` with the three tables and figure refs.
- [ ] **Step 2: Verify.** `bash paper_writing/check.sh` → `OK`.
- [ ] **Step 3: Commit.** `git add -A && git commit -m "draft emulation study"`

---

## Task 9: Simulation study & detection floor (ndnSIM)

**Files:**
- Modify: `paper_writing/sections/simulation.tex`

**Content spec (adapt `final_report.tex:752-867` + sweep setup `429-460`; ~1600–2000 words). This is the paper's core. Subsections:**
1. **Setup & sweep matrix** — 600 s runs, attack at t=300 s, N=300 catalog (live traffic), IFA black-hole producer, CP junk namespace; the sweep table (297 runs = 15/156/126); `\ref{fig:traffic-health}` (`fig_legit_traffic_health`).
2. **Detection & localization at a representative rate** — lift table (attacker post 78.4 IFA / 100 CP vs benign ~2), ~1 s latency; `\ref{fig:sim_timeseries}` (`fig_score_timeseries`), `\ref{fig:sim_spatial}` (`fig_spatial_localization`).
3. **The detection floor** — floor table (CP: 21/42/60/71/83/92/100 across 10–50 Hz; IFA: 60 at 1 Hz, ~100 from 2 Hz up); **CP ≈44 Hz, IFA ≈1.8 Hz**, quoted as bands (40–45 / 1–2 Hz); `\ref{fig:floor-headline}` (`fig_floor_headline`) and `\ref{fig:floor-pertopo}` (`fig_detection_floor`). State single-seed anchors caveat.

- [ ] **Step 1: Write simulation section** into `sections/simulation.tex` with the sweep table, lift table, floor table, and figure refs.
- [ ] **Step 2: Verify.** `bash paper_writing/check.sh` → `OK`.
- [ ] **Step 3: Commit.** `git add -A && git commit -m "draft simulation study and detection floor"`

---

## Task 10: Why the floors differ (mechanism)

**Files:**
- Modify: `paper_writing/sections/mechanism.tex`

**Content spec (adapt `final_report.tex:869-885` + `923-925`; ~700–900 words):**
- Volumetric (CP) vs qualitative (IFA): CP must out-shout the baseline (~10×) so it hides as its rate nears the crowd; IFA's timeout/NACK signature survives at ~1× baseline. `\ref{fig:mechanism}` (`fig_mechanism_contrast`).
- Feature-shift signatures: CP ~+6σ on volume/cache; IFA timeout ratio ~+3.7σ. `\ref{fig:featsig}` (`fig_feature_signature`).
- Topology invariance: `\ref{fig:heatmap}` (`fig_floor_heatmap`), row-to-row near-identical.

- [ ] **Step 1: Write mechanism section** into `sections/mechanism.tex`.
- [ ] **Step 2: Verify.** `bash paper_writing/check.sh` → `OK`.
- [ ] **Step 3: Commit.** `git add -A && git commit -m "draft floor mechanism section"`

---

## Task 11: Discussion

**Files:**
- Modify: `paper_writing/sections/discussion.tex`

**Content spec (adapt `final_report.tex:917-953`; ~900–1200 words). Condense the report's seven discussion subsections into a focused set:**
- Stealth implications of the floor gap (the security takeaway).
- Comparison with rule-based / single-metric detection (report 943–945) — blind to CP, weakened by PIT reliance.
- Cross-platform consistency & topology invariance (report 927).
- **One sentence** acknowledging a real-time monitoring implementation exists (dashboard) without detailing it.
- Limitations & future work (report 951–953): emulation IFA-only, 3 s polling averages PIT dynamics, interpolated floor bands, single-seed anchors, static IF needs retraining, temporal features to beat IFA mimicry, integrity attacks out of scope. Optional `\ref{fig:fpr}` (`fig_fpr_vs_rate`).

- [ ] **Step 1: Write discussion** into `sections/discussion.tex`.
- [ ] **Step 2: Verify.** `bash paper_writing/check.sh` → `OK`.
- [ ] **Step 3: Commit.** `git add -A && git commit -m "draft discussion"`

---

## Task 12: Conclusion + Declarations

**Files:**
- Modify: `paper_writing/sections/conclusion.tex`
- Modify: `paper_writing/sections/declarations.tex`

**Content spec:**
- **Conclusion** (adapt `final_report.tex:959-963`; ~250–350 words): recap — one unsupervised IF, two platforms; emulation corrects the IFA/PIT picture; simulation's central finding is the two mechanism-dependent floors an order of magnitude apart; the floor is the security-relevant limit.
- **Declarations** (Springer back matter, each a short paragraph): Funding (TU research grant, PI Dr. Babu R. Dawadi); Competing interests (none); Data availability (code + data bundle at the GitHub repo `github.com/ankitpokhrel08/ndn_research_work`, per `DATA.md`); Author contributions (brief CRediT-style); Acknowledgements (IOE / Pulchowk).

- [ ] **Step 1: Write conclusion and declarations** into their files.
- [ ] **Step 2: Verify.** `bash paper_writing/check.sh` → `OK`.
- [ ] **Step 3: Commit.** `git add -A && git commit -m "draft conclusion and declarations"`

---

## Task 13: Integration pass & full draft review

**Files:**
- Modify: any `sections/*.tex`, `main.tex` as needed.

- [ ] **Step 1: Full structural check.** `bash paper_writing/check.sh` → `OK: structural check passed` (all refs/cites/figures resolve; `topologies` present).
- [ ] **Step 2 (if tectonic installed): Real compile.** `cd paper_writing && tectonic main.tex` → PDF builds with no undefined references/citations in the log. If tectonic is absent, note that the user should compile once on Overleaf.
- [ ] **Step 3: Consistency sweep.** Grep the canonical numbers from Global Constraints across `sections/` and confirm no contradictory values (e.g. `grep -Rn '44' sections/` for the CP floor; `grep -Rn '1.8' sections/` for IFA). Confirm abstract, intro contributions, and conclusion state the same four claims.
- [ ] **Step 4: Length check.** `cat sections/*.tex | wc -w` → target ~7000–9000 words of body prose; trim if over.
- [ ] **Step 5: Commit.** `git add -A && git commit -m "integration pass: resolve refs, consistency, length"`
- [ ] **Step 6: Hand back to user** for a read-through before any submission-formatting polish.

---

## Self-Review (plan vs spec)

- **Spec coverage:** title/authors (Task 1), framing & IMRaD (all section tasks), 9-ref bib (Task 2), ~12 figures + topologies risk (Task 3), abstract/intro/related/method/emulation/simulation/mechanism/discussion/conclusion/declarations (Tasks 4–12), cut dashboard/arch (Task 11 one-liner), floor-band phrasing & single-seed caveat (Constraints + Tasks 9/11), Springer template + numbered refs (Task 1), declarations/data availability (Task 12). ✓ All spec sections map to a task.
- **Open risks carried from spec:** `topologies.png` missing (Task 3 Step 1 flags to user), `1060-802-1-PB1.pdf` identity (Task 2 Step 1 verifies before use), no local LaTeX engine (structural `check.sh` is the gate; tectonic optional).
- **Type/name consistency:** BibTeX keys defined in Task 2 are the exact keys cited in Tasks 5/6; figure filenames listed in Task 3 match every `\includegraphics` target in Tasks 4–11; `\label`/`\ref` pairs are co-located within each section.
- **No placeholders:** section tasks give concrete content specs (claims, canonical numbers, source line ranges, figure/ref names) rather than prose stubs; the only intentional marker is `TODO-TEMPLATE` for the preamble swap, which `check.sh` explicitly ignores.
