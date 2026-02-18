#!/usr/bin/env python3
"""Parse queue benchmark CSV output and produce a throughput bar chart."""

import csv
import sys
from pathlib import Path

import matplotlib.pyplot as plt
import numpy as np


def load(path: str) -> dict[str, dict[int, int]]:
    """Return {queue_name: {num_readers: messages_per_second}}."""
    data: dict[str, dict[int, int]] = {}
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            name = row["queue_name"]
            readers = int(row["num_readers"])
            mps = int(row["messages_per_second"])
            data.setdefault(name, {})[readers] = mps
    return data


def plot(data: dict[str, dict[int, int]], output_path: str) -> None:
    all_readers = sorted({r for d in data.values() for r in d})
    queue_names = sorted(data)

    num_queues = len(queue_names)
    bar_height = 0.7 / num_queues
    y_base = np.arange(len(all_readers), dtype=float)

    fig, ax = plt.subplots(figsize=(12, max(6, len(all_readers) * 0.55)))
    colors = plt.colormaps["tab10"].colors  # type: ignore[attr-defined]

    for queue_idx, name in enumerate(queue_names):
        offset = (queue_idx - (num_queues - 1) / 2.0) * bar_height
        values = [data[name].get(r, 0) for r in all_readers]
        ax.barh(
            y_base + offset,
            values,
            height=bar_height,
            label=name,
            color=colors[queue_idx % len(colors)],
        )

    ax.set_yticks(y_base)
    ax.set_yticklabels(all_readers)
    ax.set_xlabel("Messages per second")
    ax.set_ylabel("# Readers")
    ax.set_title("Queue Throughput")
    ax.legend()
    ax.grid(axis="x", linestyle="--", alpha=0.5)
    ax.xaxis.set_major_formatter(
        plt.FuncFormatter(
            lambda val, _: f"{val / 1e7:.1f}×10⁷" if val >= 1e7 else f"{val:,.0f}"
        )
    )

    fig.tight_layout()
    fig.savefig(output_path, dpi=150)
    print(f"Saved to {output_path}")


def main() -> None:
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <results.csv> [output.png]", file=sys.stderr)
        sys.exit(1)

    csv_path = sys.argv[1]
    png_path = (
        sys.argv[2] if len(sys.argv) > 2 else str(Path(csv_path).with_suffix(".png"))
    )

    plot(load(csv_path), png_path)


if __name__ == "__main__":
    main()
