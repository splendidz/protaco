# plot.py. Draws ../img/benchmark.svg from results.csv (bench.cpp output). Needs matplotlib.
# Run: python plot.py
# One panel per message, each with its own scale, so the smaller message is not squeezed by the larger one.
# Bars: mean ns per call, whiskers: one standard deviation over the rounds, labels: value and protobuf / protaco.
import csv
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
COLORS = {"protaco": "#1f4e79", "protaco TryView": "#6fa3d6", "protobuf": "#a6a6a6"}
TITLES = {"Fixed512": "(a) Fixed512: 512 B of fixed fields",
          "Mixed256": "(b) Mixed256: 196 B of fixed fields and two strings"}
OPERATIONS = ("encode", "decode")


def load():
    rows = {}
    with open(os.path.join(HERE, "results.csv"), encoding="utf-8") as f:
        for row in csv.DictReader(line for line in f if not line.startswith("#")):
            rows[(row["message"], row["operation"], row["library"])] = (float(row["mean_ns"]), float(row["sd_ns"]))
    return rows


def panel(ax, rows, message):
    width = 0.26
    top = 0.0
    for i, operation in enumerate(OPERATIONS):
        libraries = [lib for lib in COLORS if (message, operation, lib) in rows]
        start = i - width * (len(libraries) - 1) / 2
        protobuf = rows[(message, operation, "protobuf")][0]
        for j, lib in enumerate(libraries):
            mean, sd = rows[(message, operation, lib)]
            x = start + j * width
            ax.bar(x, mean, width * 0.92, yerr=sd, color=COLORS[lib], edgecolor="black", linewidth=0.5,
                   error_kw={"elinewidth": 0.7, "capsize": 2, "capthick": 0.7}, label=lib)
            label = "%.0f" % mean if mean >= 100 else "%.1f" % mean
            if lib != "protobuf":
                label += "\n(%.1fx)" % (protobuf / mean)
            ax.text(x, mean + sd + protobuf * 0.02, label, ha="center", va="bottom", fontsize=7.5, linespacing=1.1)
            top = max(top, mean + sd)
    ax.set_title(TITLES[message], fontsize=9, loc="left")
    ax.set_xticks(range(len(OPERATIONS)))
    ax.set_xticklabels(["Encode", "Decode"])
    ax.set_ylim(0, top * 1.22)
    ax.set_ylabel("ns per call (lower is better)")
    ax.yaxis.grid(True, linewidth=0.4, color="#d0d0d0")
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)


def main():
    plt.rcParams.update({"font.family": "serif", "font.serif": ["Times New Roman", "DejaVu Serif"], "font.size": 9,
                         "axes.linewidth": 0.6, "xtick.major.width": 0.6, "ytick.major.width": 0.6, "svg.fonttype": "path"})
    rows = load()
    fig, axes = plt.subplots(1, 2, figsize=(7.2, 2.9))
    for ax, message in zip(axes, ("Fixed512", "Mixed256")):
        panel(ax, rows, message)
    found = {}
    for ax in axes:
        for handle, label in zip(*ax.get_legend_handles_labels()):
            found.setdefault(label, handle)
    labels = [lib for lib in COLORS if lib in found]
    fig.legend([found[lib] for lib in labels], labels, loc="upper center", ncol=3, frameon=False, fontsize=8.5, bbox_to_anchor=(0.5, 1.02))
    fig.tight_layout(rect=(0, 0, 1, 0.92))
    out = os.path.join(HERE, "..", "img", "benchmark.svg")
    fig.savefig(out, format="svg", metadata={"Date": None})
    print("wrote " + os.path.normpath(out))


if __name__ == "__main__":
    main()
