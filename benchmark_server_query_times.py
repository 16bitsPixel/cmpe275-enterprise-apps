#!/usr/bin/env python3

import csv
import re
import subprocess
from pathlib import Path
from statistics import mean

TARGET = "127.0.0.1:50051"
CLIENT = Path("build/src/query_client")
OUT_DIR = Path("test_results")

MAX_ROWS = 10
RUNS_PER_CASE = 5

CASES = [
    ("Q1", "No filter", []),
    ("Q2", "Payment type = 1", ["--payment-type", "1"]),
    ("Q3", "Distance 0-1 mile", ["--distance-lo", "0", "--distance-hi", "1"]),
    ("Q4", "Total $20-$50", ["--total-lo", "2000", "--total-hi", "5000"]),
    (
        "Q5",
        "Combined filter",
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
            f"Command failed: {' '.join(map(str, args))}\n\n"
            f"STDOUT:\n{result.stdout}\n\nSTDERR:\n{result.stderr}"
        )

    return result.stdout


def extract_float(label, output):
    match = re.search(rf"{label}\s*:\s*([0-9.]+)", output)
    if not match:
        raise RuntimeError(f"Could not extract {label} from:\n{output}")
    return float(match.group(1))


def extract_request_id(output):
    match = re.search(r"request_id\s*:\s*(\S+)", output)
    if not match:
        raise RuntimeError(f"Could not extract request_id from:\n{output}")
    return match.group(1)


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
    submit_ms = extract_float("submit_ms", submit_output)

    fetch_cmd = [
        str(CLIENT),
        "--target",
        TARGET,
        "--fetch-all",
        "--request-id",
        request_id,
        "--max-rows",
        str(MAX_ROWS),
    ]

    fetch_output = run_cmd(fetch_cmd)
    fetch_all_ms = extract_float("fetch_all_ms", fetch_output)

    return {
        "query_case": query_case,
        "filter": filter_label,
        "submit_ms": submit_ms,
        "fetch_all_ms": fetch_all_ms,
    }


def main():
    if not CLIENT.exists():
        raise SystemExit(f"Could not find {CLIENT}. Build first.")

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
                f"submit_ms={result['submit_ms']:.3f}, "
                f"fetch_all_ms={result['fetch_all_ms']:.3f}"
            )

        summary_rows.append(
            {
                "Query Case": query_case,
                "Filter": filter_label,
                "Mean Submit Time (ms)": round(mean(r["submit_ms"] for r in runs), 3),
                "Mean Server Fetch-All Time (ms)": round(mean(r["fetch_all_ms"] for r in runs), 3),
            }
        )

    output_file = OUT_DIR / "benchmark_server_query_times.csv"

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