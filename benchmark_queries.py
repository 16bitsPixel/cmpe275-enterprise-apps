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

MAX_ROWS = 10
FETCHES_PER_RUN = 40
RUNS_PER_CASE = 5

CASES = [
    ("Q1", "No filter", []),
    ("Q2", "Payment type = 1", ["--payment-type", "1"]),
    ("Q3", "Distance 0-1 mile", ["--distance-lo", "0", "--distance-hi", "1"]),
    ("Q4", "Total $20-$50", ["--total-lo", "2000", "--total-hi", "5000"]),
    (
        "Q5",
        "Payment type = 1, distance 1-3 miles, total $10-$30",
        [
            "--payment-type", "1",
            "--distance-lo", "1",
            "--distance-hi", "3",
            "--total-lo", "1000",
            "--total-hi", "3000",
        ],
    ),
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
            f"Command failed: {' '.join(map(str, args))}\n\nSTDOUT:\n{result.stdout}\n\nSTDERR:\n{result.stderr}"
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


def extract_sources(output):
    return re.findall(r"source=([A-Z])", output)


def run_once(query_case, filter_label, args):
    submit_cmd = [
        str(CLIENT),
        "--target",
        TARGET,
        "--submit",
        "--chunk-size",
        str(MAX_ROWS),
        *args,
    ]

    submit_output = run_cmd(submit_cmd)
    request_id = extract_request_id(submit_output)

    total_rows = 0
    sources = set()

    start = time.perf_counter()

    for _ in range(FETCHES_PER_RUN):
        fetch_cmd = [
            str(CLIENT),
            "--target",
            TARGET,
            "--fetch",
            "--request-id",
            request_id,
            "--max-rows",
            str(MAX_ROWS),
        ]

        fetch_output = run_cmd(fetch_cmd)
        total_rows += extract_rows_returned(fetch_output)
        sources.update(extract_sources(fetch_output))

    total_fetch_ms = (time.perf_counter() - start) * 1000.0
    avg_fetch_latency_ms = total_fetch_ms / FETCHES_PER_RUN

    return {
        "query_case": query_case,
        "filter": filter_label,
        "total_fetch_ms": total_fetch_ms,
        "avg_fetch_latency_ms": avg_fetch_latency_ms,
        "rows_returned": total_rows,
        "sources": sources,
    }


def main():
    if not CLIENT.exists():
        raise SystemExit("Could not find build/src/query_client. Run ./build_mini2.sh first.")

    OUT_DIR.mkdir(exist_ok=True)

    summary_rows = []

    for query_case, filter_label, args in CASES:
        print(f"\nRunning {query_case}: {filter_label}")

        runs = []

        for run_no in range(1, RUNS_PER_CASE + 1):
            result = run_once(query_case, filter_label, args)
            runs.append(result)

            print(
                f"  run {run_no}: "
                f"fetch_ms={result['total_fetch_ms']:.2f}, "
                f"avg_fetch_ms={result['avg_fetch_latency_ms']:.2f}, "
                f"rows={result['rows_returned']}, "
                f"sources={','.join(sorted(result['sources']))}"
            )

        all_sources = set()
        for run in runs:
            all_sources.update(run["sources"])

        summary_rows.append(
            {
                "Query Case": query_case,
                "Filter": filter_label,
                "Runs": RUNS_PER_CASE,
                "Fetches per Run": FETCHES_PER_RUN,
                "Max Rows per Fetch": MAX_ROWS,
                "Mean Total Fetch Time (ms)": round(mean(r["total_fetch_ms"] for r in runs), 2),
                "Avg Fetch Latency (ms)": round(mean(r["avg_fetch_latency_ms"] for r in runs), 2),
                "Mean Rows Returned": round(mean(r["rows_returned"] for r in runs), 1),
                "Sources Observed": ",".join(sorted(all_sources)),
            }
        )

    output_file = OUT_DIR / "benchmark_query_summary.csv"

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