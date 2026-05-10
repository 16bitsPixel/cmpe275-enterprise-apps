#!/usr/bin/env python3

import csv
import re
from pathlib import Path
from statistics import mean

INPUT_FILE = Path("test_results/node_A_server_metrics.log")
OUTPUT_FILE = Path("test_results/node_A_server_by_query_case.csv")

RUNS_PER_CASE = 5

QUERY_CASES = [
    ("Q1", "No filter"),
    ("Q2", "Payment type = 1"),
    ("Q3", "Distance 0-1 mile"),
    ("Q4", "Total $20-$50"),
    ("Q5", "Combined filter"),
]

PATTERN = re.compile(
    r"\[server metrics\]\s+(\w+).*?node=(\w+).*?request_id=([^\s]+).*?server_ms=([0-9.]+)"
)


def request_number(request_id):
    return int(request_id.split("-")[1])


def query_case_for_request(request_id):
    num = request_number(request_id)
    index = (num - 1) // RUNS_PER_CASE
    if index < 0 or index >= len(QUERY_CASES):
        return None
    return QUERY_CASES[index]


def main():
    if not INPUT_FILE.exists():
        raise SystemExit(f"Missing input file: {INPUT_FILE}")

    submit_by_case = {}
    fetch_by_case = {}
    fetch_by_request = {}

    with INPUT_FILE.open("r", errors="ignore") as f:
        for line in f:
            match = PATTERN.search(line)
            if not match:
                continue

            rpc_name = match.group(1)
            request_id = match.group(3)
            server_ms = float(match.group(4))

            case_info = query_case_for_request(request_id)
            if case_info is None:
                continue

            key = case_info

            if rpc_name == "SubmitQuery":
                submit_by_case.setdefault(key, []).append(server_ms)
            elif rpc_name == "FetchChunk":
                fetch_by_case.setdefault(key, []).append(server_ms)
                fetch_by_request.setdefault(request_id, []).append(server_ms)

    rows = []

    for query_case, filter_label in QUERY_CASES:
        key = (query_case, filter_label)

        submit_times = submit_by_case.get(key, [])
        fetch_times = fetch_by_case.get(key, [])

        request_ids = [
            rid for rid in fetch_by_request
            if query_case_for_request(rid) == key
        ]

        fetch_all_times = []
        fetch_call_counts = []

        for rid in request_ids:
            values = fetch_by_request[rid]
            fetch_all_times.append(sum(values))
            fetch_call_counts.append(len(values))

        rows.append({
            "Query Case": query_case,
            "Filter": filter_label,
            "Mean Submit Time (ms)": round(mean(submit_times), 3) if submit_times else 0,
            "Mean FetchChunk Time (ms)": round(mean(fetch_times), 3) if fetch_times else 0,
            "Mean Fetch Calls": round(mean(fetch_call_counts), 2) if fetch_call_counts else 0,
            "Mean Server Fetch-All Time (ms)": round(mean(fetch_all_times), 3) if fetch_all_times else 0,
        })

    OUTPUT_FILE.parent.mkdir(exist_ok=True)

    with OUTPUT_FILE.open("w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)

    print(f"Wrote {OUTPUT_FILE}")
    for row in rows:
        print(row)


if __name__ == "__main__":
    main()