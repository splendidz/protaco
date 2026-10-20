# plot.py. Draws the figure and the table of the README from results.csv (bench.cpp output). Needs matplotlib.
# Run: python plot.py
# Output: ../img/benchmark.svg, benchmark_table.svg (English) and benchmark.ko.svg, benchmark_table.ko.svg (Korean).
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
MESSAGES = ("Fixed512", "Mixed256")
OPERATIONS = ("encode", "decode")
TEXT = {
    "en": {
        "suffix": "",
        "title": {"Fixed512": "(a) Fixed-length, 512 B", "Mixed256": "(b) Variable-length, 256 B (196 B fixed + two strings)"},
        "message": {"Fixed512": "Fixed-length, 512 B", "Mixed256": "Variable-length, 256 B"},
        "library": {"protaco": "Protaco", "protaco TryView": "Protaco TryView (zero-copy)", "protobuf": "Protocol Buffers"},
        "operation": {"encode": "Encoding", "decode": "Decoding", "view": "Decoding, TryView (zero-copy)"},
        "ylabel": "ns per call (lower is better)",
        "header": ("Message", "Operation", "Protaco", "Protocol Buffers", "Speedup"),
        "unsupported": "Not supported",
        "caption": "Table 1. Time per call in ns, mean ± s.d. over 20 rounds of 100,000 calls. Lower is better.",
        "note": "Speedup: Protocol Buffers / Protaco. † Compared with Protocol Buffers decoding.",
    },
    "ko": {
        "suffix": ".ko",
        "title": {"Fixed512": "(a) 고정 길이 512 B", "Mixed256": "(b) 가변 길이 256 B (고정 196 B + 문자열 2개)"},
        "message": {"Fixed512": "고정 길이 512 B", "Mixed256": "가변 길이 256 B"},
        "library": {"protaco": "Protaco", "protaco TryView": "Protaco TryView (zero-copy)", "protobuf": "Protocol Buffers"},
        "operation": {"encode": "Encoding", "decode": "Decoding", "view": "Decoding, TryView (zero-copy)"},
        "ylabel": "호출당 ns (낮을수록 좋음)",
        "header": ("메시지", "동작", "Protaco", "Protocol Buffers", "배율"),
        "unsupported": "지원 안 함",
        "caption": "표 1. 호출당 시간(ns), 100,000회 호출 20라운드의 평균 ± 표준편차. 낮을수록 좋음.",
        "note": "배율: Protocol Buffers / Protaco. † Protocol Buffers Decoding과 비교.",
    },
}


def load():
    rows = {}
    with open(os.path.join(HERE, "results.csv"), encoding="utf-8") as f:
        for row in csv.DictReader(line for line in f if not line.startswith("#")):
            rows[(row["message"], row["operation"], row["library"])] = (float(row["mean_ns"]), float(row["sd_ns"]))
    return rows


def number(value):
    return "%.0f" % value if value >= 100 else "%.1f" % value


def panel(ax, rows, message, text):
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
                   error_kw={"elinewidth": 0.7, "capsize": 2, "capthick": 0.7}, label=text["library"][lib])
            label = number(mean)
            if lib != "protobuf":
                label += "\n(%.1f×)" % (protobuf / mean)
            ax.text(x, mean + sd + protobuf * 0.02, label, ha="center", va="bottom", fontsize=7.5, linespacing=1.1)
            top = max(top, mean + sd)
    ax.set_title(text["title"][message], fontsize=9, loc="left")
    ax.set_xticks(range(len(OPERATIONS)))
    ax.set_xticklabels([text["operation"][op] for op in OPERATIONS])
    ax.set_ylim(0, top * 1.22)
    ax.set_ylabel(text["ylabel"])
    ax.yaxis.grid(True, linewidth=0.4, color="#d0d0d0")
    ax.set_axisbelow(True)
    for side in ("top", "right"):
        ax.spines[side].set_visible(False)


def figure(rows, text):
    fig, axes = plt.subplots(1, 2, figsize=(7.2, 2.9))
    for ax, message in zip(axes, MESSAGES):
        panel(ax, rows, message, text)
    found = {}
    for ax in axes:
        for handle, label in zip(*ax.get_legend_handles_labels()):
            found.setdefault(label, handle)
    labels = [text["library"][lib] for lib in COLORS if text["library"][lib] in found]
    fig.legend([found[label] for label in labels], labels, loc="upper center", ncol=3, frameon=False, fontsize=8.5, bbox_to_anchor=(0.5, 1.02))
    fig.tight_layout(rect=(0, 0, 1, 0.92))
    save(fig, "benchmark" + text["suffix"] + ".svg")


# One row per (message, operation): label cells, then protaco, protobuf and speedup cells.
def table_rows(rows, text):
    out = []
    for message in MESSAGES:
        first = True
        for operation, lib in (("encode", "protaco"), ("decode", "protaco"), ("decode", "protaco TryView")):
            if (message, operation, lib) not in rows:
                continue
            mean, sd = rows[(message, operation, lib)]
            base = rows[(message, operation, "protobuf")][0]
            view = lib == "protaco TryView"
            other = text["unsupported"] if view else "%.1f ± %.1f" % (base, rows[(message, operation, "protobuf")][1])
            speedup = "%.1f×" % (base / mean) + ("†" if view else "")
            label = text["operation"]["view" if view else operation]
            out.append((text["message"][message] if first else "", label, "%.1f ± %.1f" % (mean, sd), other, speedup))
            first = False
        out.append(None)   # rule between messages
    return out[:-1]


def table(rows, text):
    body = table_rows(rows, text)
    lines = len(body) + 1
    fig = plt.figure(figsize=(7.2, 0.62 + 0.235 * lines))
    ax = fig.add_axes((0.02, 0.0, 0.96, 1.0))
    ax.set_axis_off()
    height = fig.get_figheight()
    step = 0.235 / height
    y = 1.0 - 0.2 / height
    ax.text(0.0, y, text["caption"], ha="left", va="center", fontsize=8.5)
    columns = ((0.0, "left"), (0.25, "left"), (0.66, "right"), (0.86, "right"), (1.0, "right"))
    y -= step * 0.85
    ax.axhline(y, color="black", linewidth=0.9)
    y -= step * 0.55
    for (x, align), cell in zip(columns, text["header"]):
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
                    color="#555555" if cell == text["unsupported"] else "black")
        y -= step * 0.45
    y -= step * 0.15
    ax.axhline(y, color="black", linewidth=0.9)
    ax.text(0.0, y - step * 0.6, text["note"], ha="left", va="center", fontsize=7.5)
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    save(fig, "benchmark_table" + text["suffix"] + ".svg")


def save(fig, name):
    out = os.path.join(IMG, name)
    fig.savefig(out, format="svg", metadata={"Date": None})
    plt.close(fig)
    print("wrote " + os.path.normpath(out))


def main():
    plt.rcParams.update({"font.family": ["Times New Roman", "DejaVu Serif", "NanumMyeongjo", "Batang", "Noto Serif KR"], "font.size": 9,
                         "axes.linewidth": 0.6, "xtick.major.width": 0.6, "ytick.major.width": 0.6, "svg.fonttype": "path",
                         "svg.hashsalt": "protaco"})
    rows = load()
    for text in TEXT.values():
        figure(rows, text)
        table(rows, text)


if __name__ == "__main__":
    main()
