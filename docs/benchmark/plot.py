# plot.py. Draws the figure and the table of the README from results.csv (bench.cpp output). Needs matplotlib.
# Run: python plot.py
# Output: ../img/benchmark.svg (figure) and ../img/benchmark_table.svg (table). Both READMEs use these English images.
# Figure: one panel per message, each with its own scale, so the smaller message is not squeezed by the larger one.
# Bars: mean ns per call, whiskers: one standard deviation over the rounds, labels: value and protobuf / protaco.
# Table: booktabs style (three rules, no vertical lines), the same numbers with the speedup.
import csv
import os

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
IMG = os.path.join(HERE, "..", "img")
COLORS = {"protaco": "#1f4e79", "protaco TryView": "#6fa3d6", "protobuf": "#a6a6a6"}
LIBRARIES = {"protaco": "Protaco", "protaco TryView": "Protaco TryView (zero-copy)", "protobuf": "Protocol Buffers"}
MESSAGES = ("Fixed512", "Mixed256")
TITLES = {"Fixed512": "(a) Fixed-length, 512 B", "Mixed256": "(b) Variable-length, 256 B (196 B fixed + two strings)"}
NAMES = {"Fixed512": "Fixed-length, 512 B", "Mixed256": "Variable-length, 256 B"}
OPERATIONS = ("encode", "decode")
# Table rows: (message, operation, library, label). Fixed-length encoding writes the frame in place.
ROWS = (("Fixed512", "encode", "protaco", "Encoding (zero-copy)"),
        ("Fixed512", "decode", "protaco", "Decoding"),
        ("Fixed512", "decode", "protaco TryView", "Decoding, TryView (zero-copy)"),
        ("Mixed256", "encode", "protaco", "Encoding"),
        ("Mixed256", "decode", "protaco", "Decoding"))
HEADER = ("Message", "Operation", "Protaco (ns)", "Protocol Buffers (ns)", "Speedup")
CAPTION = "Table 1. Time per call, mean ± s.d. over 20 rounds of 100,000 calls. Lower is better."
NOTE = "Speedup: Protocol Buffers / Protaco. † Compared with Protocol Buffers decoding."


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
                   error_kw={"elinewidth": 0.7, "capsize": 2, "capthick": 0.7}, label=LIBRARIES[lib])
            label = "%.0f" % mean if mean >= 100 else "%.1f" % mean
            if lib != "protobuf":
                label += "\n(%.1f×)" % (protobuf / mean)
            ax.text(x, mean + sd + protobuf * 0.02, label, ha="center", va="bottom", fontsize=7.5, linespacing=1.1)
            top = max(top, mean + sd)
    ax.set_title(TITLES[message], fontsize=9, loc="left")
    ax.set_xticks(range(len(OPERATIONS)))
    ax.set_xticklabels(["Encoding", "Decoding"])
    ax.set_ylim(0, top * 1.22)
    ax.set_ylabel("ns per call (lower is better)")
    ax.yaxis.grid(True, linewidth=0.4, color="#d0d0d0")
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)


def figure(rows):
    fig, axes = plt.subplots(1, 2, figsize=(7.2, 2.9))
    for ax, message in zip(axes, MESSAGES):
        panel(ax, rows, message)
    found = {}
    for ax in axes:
        for handle, label in zip(*ax.get_legend_handles_labels()):
            found.setdefault(label, handle)
    labels = [LIBRARIES[lib] for lib in COLORS if LIBRARIES[lib] in found]
    fig.legend([found[label] for label in labels], labels, loc="upper center", ncol=3, frameon=False, fontsize=8.5, bbox_to_anchor=(0.5, 1.02))
    fig.tight_layout(rect=(0, 0, 1, 0.92))
    save(fig, "benchmark.svg")


# Cells per row; None marks the rule between messages.
def table_rows(rows):
    out = []
    previous = None
    for message, operation, lib, label in ROWS:
        if previous is not None and message != previous:
            out.append(None)
        mean, sd = rows[(message, operation, lib)]
        base, base_sd = rows[(message, operation, "protobuf")]
        view = lib == "protaco TryView"
        other = "Not supported" if view else "%.1f ± %.1f" % (base, base_sd)
        speedup = "%.1f×" % (base / mean) + ("†" if view else "")
        out.append((NAMES[message] if message != previous else "", label, "%.1f ± %.1f" % (mean, sd), other, speedup))
        previous = message
    return out


def table(rows):
    body = table_rows(rows)
    fig = plt.figure(figsize=(7.2, 0.62 + 0.235 * (len(body) + 1)))
    ax = fig.add_axes((0.02, 0.0, 0.96, 1.0))
    ax.set_axis_off()
    height = fig.get_figheight()
    step = 0.235 / height
    y = 1.0 - 0.2 / height
    ax.text(0.0, y, CAPTION, ha="left", va="center", fontsize=8.5)
    columns = ((0.0, "left"), (0.25, "left"), (0.66, "right"), (0.86, "right"), (1.0, "right"))
    y -= step * 0.85
    ax.axhline(y, color="black", linewidth=0.9)
    y -= step * 0.55
    for (x, align), cell in zip(columns, HEADER):
        ax.text(x, y, cell, ha=align, va="center", fontsize=9)
    y -= step * 0.55
    ax.axhline(y, color="black", linewidth=0.5)
    for row in body:
        if row is None:
            y -= step * 0.2
            ax.plot((0.0, 1.0), (y, y), color="black", linewidth=0.3, transform=ax.transAxes)
            y -= step * 0.2
            continue
        y -= step * 0.55
        for index, ((x, align), cell) in enumerate(zip(columns, row)):
            ax.text(x, y, cell, ha=align, va="center", fontsize=9, fontweight="bold" if index == 2 else "normal",
                    color="#555555" if cell == "Not supported" else "black")
        y -= step * 0.45
    y -= step * 0.15
    ax.axhline(y, color="black", linewidth=0.9)
    ax.text(0.0, y - step * 0.6, NOTE, ha="left", va="center", fontsize=7.5)
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    save(fig, "benchmark_table.svg")


def save(fig, name):
    out = os.path.join(IMG, name)
    fig.savefig(out, format="svg", metadata={"Date": None})
    plt.close(fig)
    print("wrote " + os.path.normpath(out))


def main():
    plt.rcParams.update({"font.family": "serif", "font.serif": ["Times New Roman", "DejaVu Serif"], "font.size": 9,
                         "axes.linewidth": 0.6, "xtick.major.width": 0.6, "ytick.major.width": 0.6, "svg.fonttype": "path",
                         "svg.hashsalt": "protaco"})
    rows = load()
    figure(rows)
    table(rows)


if __name__ == "__main__":
    main()
