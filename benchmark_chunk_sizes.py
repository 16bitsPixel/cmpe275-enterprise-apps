#!/usr/bin/env python3

import csv
import re
import subprocess
import time
from pathlib import Path
from statistics import mean

TARGET = "127.0.0.1:50051"
CLIENT = Path("build/src/query_client")
OUT_DIR = Path("test_results")

RUNS_PER_CASE = 5
TOTAL_SAMPLE_ROWS = 400

CHUNK_CASES = [
    ("C1", 10),
    ("C2", 50),
    ("C3", 100),
]


def run_cmd(args):
    result = subprocess.run(
        args,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    if result.returncode != 0:
        raise RuntimeError(
            f"Command failed: {' '.join(map(str, args))}\n\n"
            f"STDOUT:\n{result.stdout}\n\n"
            f"STDERR:\n{result.stderr}"
        )

    return result.stdout


def extract_request_id(output):
    match = re.search(r"request_id\s*:\s*(\S+)", output)
    if not match:
        raise RuntimeError(f"Could not extract request_id from:\n{output}")
    return match.group(1)


def extract_rows_returned(output):
    match = re.search(r"rows_returned\s*:\s*(\d+)", output)
    return int(match.group(1)) if match else 0


def extract_done(output):
    match = re.search(r"done\s*:\s*(true|false)", output)
    return bool(match and match.group(1) == "true")


def extract_sources(output):
    return re.findall(r"source=([A-Z])", output)


def run_once(max_rows):
    submit_output = run_cmd([
        str(CLIENT),
        "--target", TARGET,
        "--submit",
        "--chunk-size", str(max_rows),
    ])

    request_id = extract_request_id(submit_output)

    total_rows = 0
    fetch_count = 0
    sources = set()

    start = time.perf_counter()

    while total_rows < TOTAL_SAMPLE_ROWS:
        fetch_output = run_cmd([
            str(CLIENT),
            "--target", TARGET,
            "--fetch",
            "--request-id", request_id,
            "--max-rows", str(max_rows),
        ])

        rows = extract_rows_returned(fetch_output)
        done = extract_done(fetch_output)

        total_rows += rows
        fetch_count += 1
        sources.update(extract_sources(fetch_output))

        if done or rows == 0:
            break

    total_ms = (time.perf_counter() - start) * 1000.0
    avg_fetch_ms = total_ms / fetch_count if fetch_count else 0.0

    return {
        "fetch_count": fetch_count,
        "total_rows": total_rows,
        "total_ms": total_ms,
        "avg_fetch_ms": avg_fetch_ms,
        "sources": sources,
    }


def main():
    if not CLIENT.exists():
        raise SystemExit("Could not find build/src/query_client. Run ./build_mini2.sh first.")

    OUT_DIR.mkdir(exist_ok=True)
    summary_rows = []

    for case_id, max_rows in CHUNK_CASES:
        print(f"\nRunning {case_id}: max_rows={max_rows}")

        runs = []

        for run_no in range(1, RUNS_PER_CASE + 1):
            result = run_once(max_rows)
            runs.append(result)

            print(
                f"  run {run_no}: "
                f"fetches={result['fetch_count']}, "
                f"rows={result['total_rows']}, "
                f"total_ms={result['total_ms']:.2f}, "
                f"avg_fetch_ms={result['avg_fetch_ms']:.2f}, "
                f"sources={','.join(sorted(result['sources']))}"
            )

        all_sources = set()
        for run in runs:
            all_sources.update(run["sources"])

        summary_rows.append({
            "Max Rows per Fetch": max_rows,
            "Mean Fetch Calls": round(mean(r["fetch_count"] for r in runs), 1),
            "Mean Fetch Time (ms)": round(mean(r["total_ms"] for r in runs), 2),
            "Avg Fetch Latency (ms)": round(mean(r["avg_fetch_ms"] for r in runs), 2),
            "Sources Observed": ",".join(sorted(all_sources)),
        })

    output_file = OUT_DIR / "benchmark_chunk_sizes_summary.csv"

    with open(output_file, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=summary_rows[0].keys())
        writer.writeheader()
        writer.writerows(summary_rows)

    print(f"\nWrote {output_file}")
    print("\nSummary:")
    for row in summary_rows:
        print(row)


if __name__ == "__main__":
    main()