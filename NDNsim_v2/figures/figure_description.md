# NDNsim v2 — figure descriptions (first rate sweep, BROKEN)

v2 is the **first rate-sweep attempt and it is broken** — the legitimate background traffic
died ~30 s into every run, so its detection numbers are measured against a silent network and
**must not be cited**. It is kept for the methodology narrative ("why a third sweep was
needed"). Final, correct results are in
[`../../NDNsim_v4/figures/figure_description.md`](../../NDNsim_v4/figures/figure_description.md).

## The one worth mentioning (paper / methodology)

### `diagnostic/fig_traffic_decay.png` ⭐
Legitimate consumer interest rate vs time, all three topologies, attack window (t ≥ 300)
shaded. **Tells:** the background collapses to ~0 by t≈30 s because `NumberOfContents = 100`
exhausts the Zipf catalog — so the **entire attack window runs against a dead network**. This
is the figure that justifies the catalog fix (300) used from v3 on. *Caption:* "Dead-traffic
bug in the first sweep: legitimate consumers fall silent long before the attack starts, so
detection there is measured against silence, not live traffic."

## Other figures (kept, but confounded — do not cite as results)

| file | what it is | why not citable |
|---|---|---|
| `detection/fig_detection_floor.png` | the v2 "detection floor" curves | floor measured against silence; CP bend at 12–15 Hz is an artifact |
| `diagnostic/fig_legit_traffic_decay.png` | original 3-panel traffic diagnostic | superseded by the cleaner `fig_traffic_decay.png` above |

See [`../NEWDATA_FINDINGS.md`](../NEWDATA_FINDINGS.md) for the full write-up of the bug and the fix.
