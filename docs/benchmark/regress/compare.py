# compare.py <work folder>. Reads <lang>_<old|new>_<run>.txt written by regress.cmd and prints, per language and case,
# the median over the runs of each side and the change from old to new.
import glob
import os
import statistics
import sys


def load(work, lang, side):
    values = {}
    order = []
    for path in sorted(glob.glob(os.path.join(work, "%s_%s_*.txt" % (lang, side)))):
        with open(path, encoding="utf-8") as f:
            for line in f:
                name, ns = line.split()
                if name not in values:
                    values[name] = []
                    order.append(name)
                values[name].append(float(ns))
    return order, values


def main():
    work = sys.argv[1]
    for lang in ("cpp", "cs", "py"):
        order, old = load(work, lang, "old")
        _, new = load(work, lang, "new")
        if not order:
            continue
        print()
        print("%-6s %-28s %10s %10s %8s" % (lang, "case", "old ns", "new ns", "change"))
        for name in order:
            o = statistics.median(old[name])
            n = statistics.median(new[name])
            print("%-6s %-28s %10.2f %10.2f %+7.1f%%" % ("", name, o, n, (n - o) / o * 100))


if __name__ == "__main__":
    main()
