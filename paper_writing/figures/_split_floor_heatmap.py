"""Split the combined fig_floor_heatmap.png into two separate per-attack heatmaps.

Reproduces the styling of Fig 3 in NDNsim_v4/notebooks/02_detection_floor.ipynb,
but emits one standalone figure per attack (CP and IFA).
"""
from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib as mpl
import matplotlib.pyplot as plt

mpl.rcParams.update({
    "savefig.dpi": 200, "savefig.bbox": "tight",
    "font.size": 11, "axes.titlesize": 12, "axes.titleweight": "bold",
    "axes.labelsize": 11,
})

HERE = Path(__file__).resolve().parent
CSV = HERE.parent.parent / "NDNsim_v4" / "processed" / "detection_floor.csv"
floor = pd.read_csv(CSV)

TOPOS = ["tree", "dfn", "dumbbell"]
ATK_LB = {"cp": "Cache Pollution (CP)", "ifa": "Interest Flooding (IFA)"}

for atk in ["cp", "ifa"]:
    piv = (floor[floor.attack == atk]
           .pivot_table(index="topo", columns="rate", values="detection")
           .reindex(TOPOS))

    fig, ax = plt.subplots(figsize=(6.8, 3.6))
    im = ax.imshow(piv.values, aspect="auto", cmap="RdYlGn", vmin=0, vmax=100)
    ax.set_xticks(range(len(piv.columns)))
    ax.set_xticklabels(piv.columns.astype(int), fontsize=8)
    ax.set_yticks(range(len(piv.index)))
    ax.set_yticklabels(piv.index)
    for i in range(piv.shape[0]):
        for j in range(piv.shape[1]):
            v = piv.values[i, j]
            if np.isfinite(v):
                ax.text(j, i, f"{v:.0f}", ha="center", va="center", fontsize=7,
                        color="black" if 25 < v < 88 else "white")
    ax.set_xlabel("attacker rate (interests/s)")
    ax.set_title(f"{ATK_LB[atk]} — detection % across rate and topology")
    fig.colorbar(im, ax=ax, fraction=0.046, pad=0.03, label="detection %")

    out = HERE / f"fig_floor_heatmap_{atk}.png"
    fig.savefig(out)
    print("wrote", out)
    plt.close(fig)
