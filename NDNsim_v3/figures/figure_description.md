# NDNsim v3 — figure descriptions (catalog-fixed sweep, interpolated floor)

v3 is where the data became honest (`NumberOfContents` 100 → **300** revived the background
traffic), so it recovered the *shape* of both detection floors — but the CP 90% crossing was
only **interpolated** across an unsampled r30→r50 gap, and IFA looked **flat** because the
sweep stopped at 10 Hz. v4 closed both gaps. Final results:
[`../../NDNsim_v4/figures/figure_description.md`](../../NDNsim_v4/figures/figure_description.md).

## The one worth mentioning (paper / methodology)

### `detection/fig_floor_gaps.png` ⭐
CP and IFA detection curves (pooled over topologies, log-rate axis) with the **two measurement
gaps highlighted**: the red band (r30→r50) is where the CP 90% crossing lives but no rates were
sampled, and the blue band (< 10 Hz) is the region IFA was never probed in — which is why it
*looks* floor-less. **Tells:** v3 established the floor's shape and a real FPR, but could not pin
either crossing; this is the direct motivation for the v4 top-up (CP r35/40/45, IFA r1–r8).
*Caption:* "The catalog-fixed sweep recovers the floor's shape but leaves the CP crossing
interpolated and the IFA floor unprobed — motivating the targeted top-up runs."

## Other figures (kept)

| file | what it is | note |
|---|---|---|
| `detection/fig_detection_floor.png` | the v3 per-topology floor curves | correct shape; crossing interpolated (see ⭐ above) |
| `diagnostic/fig_legit_traffic_health.png` | legit traffic alive for the full 600 s | the catalog fix working — same check as v4 |

See [`../BESTDATA_FINDINGS.md`](../BESTDATA_FINDINGS.md) for the full write-up.
