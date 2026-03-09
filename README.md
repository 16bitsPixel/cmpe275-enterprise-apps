# NYC Yellow Taxi Trip Records – Streaming vs In-Memory Query Engine (Phases 1–3)

## Overview

This project builds and benchmarks a C++ query engine over the **NYC OpenData Yellow Taxi Trip Records** dataset. The dataset spans multiple years (2019–2022) and totals **~24.1GB on disk** with **100M+ rows**. The goal is to ingest and query large CSV datasets efficiently, then measure how different design choices affect performance.

Two approaches are implemented **side-by-side** across three phases:

1. **In-Memory Engine** – ingest all rows into memory, then run fast queries over the stored dataset  
2. **Streaming Engine** – do not store the dataset; scan files and evaluate predicates row-by-row per query (bounded memory)

The project is organized into **three phases** that progressively optimize performance:

- **Phase 1 – Serial baseline**
- **Phase 2 – OpenMP parallelization**
- **Phase 3 – Data model redesign (Object-of-Arrays / SoA)**

---

## Dataset

Source: NYC OpenData – Yellow Taxi Trip Records  
Example: 2019 Yellow Taxi Trip Data (NYC OpenData)

The schema contains 18 columns including:
- Pickup / dropoff datetime (strings in CSV, converted to epoch UTC during parsing)
- Passenger count
- Trip distance
- Pickup/dropoff locations
- Payment type
- Fare/extra/tax/tip/total amounts

---

## What the Project Does

### 1) Ingestion
Reads one or more CSV files from a directory and parses each row into primitive values.  
Datetime strings are converted into **epoch milliseconds (UTC)** for fast comparisons in queries.

### 2) Querying
Supports predicate-based queries over the dataset. Queries typically filter by:
- Pickup / dropoff time range
- Trip distance range
- Total amount range
- Tip amount range
- Payment type

Two query modes are supported:
- **count** – return number of matching rows  
- **execute** – return the matching row set (references or row indices depending on phase)

### 3) Benchmarking
Includes a benchmark system that runs ingestion and querying multiple times (10+ runs) and reports:
- Ingestion time
- Count query time
- Execute query time
- Rows read / scanned / matched
- Parse failure rate
- Selectivity
- Row throughput (rows/sec)
- Throughput (MiB/sec)
- Estimated data scanned (AoS vs SoA)
