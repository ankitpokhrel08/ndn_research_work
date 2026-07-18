# Design Spec — Journal Paper for *Discover Networks*

**Date:** 2026-07-18
**Target venue:** *Discover Networks* (Springer, open access, ISSN 3004-9792) — https://link.springer.com/journal/44354
**Deliverable:** first-draft LaTeX manuscript in `paper_writing/`
**Source material:** `final_report.tex` (IOE thesis, 973 lines), `Figures/` (28 figs), `Citations/` (9 reference PDFs)

---

## 1. Title & framing

**Title (final, per author):**
> Detecting Network Anomalies in Named Data Networking Through Isolation Forest-Based Behavioral Analysis

**Core thesis (headline contribution):** The security-relevant limit of behavioral anomaly detection in NDN is not peak accuracy but each attack's **detection floor** — the attacker rate below which the attack is indistinguishable from legitimate traffic. Because Interest Flooding (IFA) has a *qualitative* signature (timeouts/NACKs) while Cache Pollution (CP) has a *volumetric* one, their floors sit an order of magnitude apart (IFA ≈ 1.8 Hz, CP ≈ 44 Hz).

The paper keeps the descriptive title but the **narrative is floor-first**: the Isolation Forest detector is the method; the detection floor is the result that makes the paper novel.

## 2. Contribution stack (claimed, in order)

1. **Detection floor as the security metric** — formally defined (rate at which attacker-node detection falls below 90%); it, not peak detection, bounds attacker stealth.
2. **Two floors an order of magnitude apart, mechanistically explained** — volumetric CP must out-shout the legitimate baseline (~10×); qualitative IFA stays detectable at ~1× baseline. Floors are topology-invariant.
3. **One unsupervised Isolation Forest, cross-platform** — the same detector on real-NFD emulation (miniNDN) and simulation (ndnSIM); trained on normal traffic only.
4. **PIT-feature myth correction** — PIT counters are non-discriminating at practical polling intervals (KS D ≈ 0.03–0.10); satisfaction ratio, cache-hit ratio, and NACK rate carry the IFA signal.

## 3. Structure (standard IMRaD, modeled on `Citations/`)

Both model papers (Xing et al. 2021, IEEE Comm. Mag.; Liu et al. 2025, *Future Internet*) follow: **unstructured Abstract → Keywords → Introduction (motivation → gap → explicit contribution bullets → "the remainder of this paper…" roadmap) → Related Work → Method → Experiments/Results → Conclusion.** We follow this exactly.

| # | Section | Content | Figures |
|---|---------|---------|---------|
| — | Abstract | Unstructured, ~200 words, floor-forward | — |
| — | Keywords | NDN; Interest Flooding; Cache Pollution; Isolation Forest; anomaly detection; detection floor | — |
| 1 | Introduction | NDN + PIT/CS/FIB; IFA & CP; the "floor not peak" gap; **explicit contribution bullets**; roadmap sentence | topologies |
| 2 | Related Work | 9 refs woven into a gap argument (NDN foundations; attack/caching surveys; Isolation Forest; ML-IDS; prior NDN-IF work incl. IFDM); ends on the 4 gaps this paper closes | — |
| 3 | Method | Feature engineering (10 emu / 9 sim), IF pipeline (StandardScaler + IsolationForest, 200 trees, contamination 0.01), auto-threshold, 0–100 score normalization, **formal detection-floor definition**, evaluation metrics (DR, FPR, KS) | — |
| 4 | Emulation study (miniNDN) | IFA detection across 3 topologies; spatial fingerprint; PIT-myth via KS; **CP negative result → motivates the simulation study** | emu_score_dist, emu_pernode_{tree,dumbbell,dfn}, emu_ks_heatmap_tree, emu_feature_importance |
| 5 | Simulation study & detection floor (ndnSIM) | Live-traffic validity; high-rate detection + localization; **rate sweep → the two floors** | fig_legit_traffic_health, fig_score_timeseries, fig_spatial_localization, **fig_floor_headline**, fig_detection_floor |
| 6 | Why the floors differ | Volumetric vs qualitative mechanism; feature-shift signatures; topology invariance | fig_mechanism_contrast, fig_feature_signature, fig_floor_heatmap |
| 7 | Discussion | Stealth implications; comparison with rule-based detection; limitations & future work | fig_fpr_vs_rate (optional) |
| 8 | Conclusion | Recap of the floor finding and cross-platform result | — |

**Figure budget:** ~12 (subset of 28). Note: Sections 7 and 8 may be merged as "Discussion and Conclusion" if length warrants.

**Explicitly cut** (per approved framing): real-time dashboard, 5-layer system-architecture chapter, sequence diagram. These collapse to a single sentence in Discussion noting a live-monitoring implementation exists.

## 4. Reuse vs. rewrite

- **Reuse (adapt prose):** methodology, KS/floor results, mechanism discussion — the thesis prose is already strong.
- **Rewrite/tighten:** Abstract (Discover-style, ~200 words), Introduction (sharper floor-first hook + contribution bullets + roadmap), Related Work (journal-grade synthesis of the 9 refs, not a list).
- **Add:** formal detection-floor definition in Method; crisp contribution bullet list in Introduction.

## 5. Format & mechanics

- **Template:** official Springer Nature `sn-jnl` (`sn-article.cls`), numbered reference style (`sn-mathphys-num` or the Discover default). All new files under `paper_writing/`. Fetch/confirm the template and its exact class options at draft time.
- **Bibliography:** build `paper_writing/references.bib` from the 9 `Citations/` PDFs. Each PDF's title/authors/year/venue must be **verified from the PDF**, not assumed. Confirmed so far:
  - `anomaly_isolation_forest_ndn.pdf` → Xing, Chen, Hou, Zhou, Dong, Zeng, Luo, Ma, "Isolation Forest-Based Mechanism to Defend against Interest Flooding Attacks in NDN," *IEEE Communications Magazine*, Mar 2021.
  - `futureinternet-17-00206-v2.pdf` → Liu, Yu, Wu, Peng, "XGBoost-Based Detection of DDoS Attacks in NDN," *Future Internet* 2025, 17(5):206.
  - `Isolation_Forest.pdf` → Liu, Ting, Zhou, "Isolation Forest," ICDM 2008.
  - `Named_data_networking_NDN_project_vanjacobson.pdf` → Zhang et al., "Named Data Networking," ACM SIGCOMM CCR 2014 (NDN project / Jacobson).
  - `Security_and_Integrity_Attacks_in_Named_Data_Networking_A_Survey.pdf` → Shah et al., NDN security & integrity attacks survey.
  - `Towards Persistent Detection of DDoS Attacks in NDN.pdf` → Xu et al., sketch-based DDoS detection.
  - `ndn-0028-1-ndnsim-v2.pdf` → Mastorakis et al., ndnSIM 2 technical report (cite for the simulator).
  - `Caching_on_Named_Data_Network_a_Survey_and_Future_.pdf` → NDN caching survey (CP context) — verify authors/year.
  - `1060-802-1-PB1.pdf` → **unknown, must open and verify** before use.
- **Length:** Discover has no hard limit; target ~7–9k words, focused.
- **Authors:** Ankit Pokhrel¹, Asmit Phuyal¹, Darshan Kumar Paudyal¹, Babu R. Dawadi¹ (**corresponding**). Affiliation ¹: Department of Electronics & Computer Engineering, Pulchowk Campus, IOE, Tribhuvan University.
- **Declarations:** add the Springer-required back matter (Funding — TU research grant; Competing interests; Data availability — GitHub repo + data bundle; Author contributions).

## 6. Build assets to produce/locate

- `paper_writing/main.tex` (or `sn-article.tex`) + `sn-jnl.cls`/`sn-article.cls` from the Springer template.
- `paper_writing/references.bib` (9 verified entries).
- **`topologies.png`** is referenced by the thesis but **not present in `Figures/`** — locate in the repo or regenerate before the Introduction figure can compile.
- Copy the ~12 selected figures into `paper_writing/figures/` (or set `\graphicspath` to `../Figures/`).

## 7. Open items / risks

- `topologies.png` missing — must be found or regenerated.
- `1060-802-1-PB1.pdf` identity unverified.
- Floor crossings are interpolated between sampled rates → quote as bands (CP 40–45 Hz, IFA 1–2 Hz) in the text, exact values only in headline.
- Two highest sweep rates use a single seed — state this as a limitation.
