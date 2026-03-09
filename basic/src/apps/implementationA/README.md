# NYC Yellow Taxi – In-Memory Implementation
## Overview
This project implements an in-memory data processing system for the NYC Yellow Taxi Trip Records dataset (2019–2022). The total dataset size exceeds 24GB and contains over 100 million records.

The system was developed in three phases:

1. Phase 1 – Serial Baseline

2. Phase 2 – OpenMP Parallelization

3. Phase 3 – Object-of-Arrays (SoA) Model Redefinition

The primary goal is to evaluate ingestion and query performance under different architectural and parallelization strategies.

## Dataset

Source: NYC OpenData – Yellow Taxi Trip Records  
Years used: 2019–2022  
Total size on disk: ~24.1GB  
Average rows per year: ~30 million  
Total rows: ~109 million+  

## Architecture
### Core Components
| Component           | Responsibility                             |
| ------------------- | ------------------------------------------ |
| `CSVReader`         | Reads CSV files from a directory           |
| `TaxiTripParser`    | Converts CSV rows into primitive types     |
| `TaxiTripStoreSoA`  | Stores dataset using columnar (SoA) layout |
| `TaxiTripQuerySpec` | Defines predicate constraints              |
| `BenchmarkRunner`   | Executes ingestion + query benchmarks      |

## Phase Breakdown
## Phase 1 - Serial (Array-of-Structures)
-  records stored as std::vector<TaxiTripRecord>  
-  datetime parsed into epoch UTC (int64_t)  
-  predicate-based query API
-  10-run benchmark baseline

Characteristics:  
-  high ingestion cost (~2500s)
-  fast query (~7-8s)
-  memory-heavy (13,703 MiB scanned per query)

## Phase 2 - Parallel (OpenMP)
-  file-level parallel ingestion
-  parallel query via index chunking
-  reduction for count queries
-  two-pass output for execute queries

Findings:  
-  ingestion improved (~2.09x speedup)  
-  query degraded due to memory bandwidth contention
-  demonstrated memory-bound behavior

## Phase 3 - Object-of-Arrays (SoA)
-  replaced AoS with column vectors
-  queries access only necessary columns
-  reduced scanned data from 13703 MiB to 4453 MiB
-  query count time reduced from 10.59s to 0.843s
-  ~12.6x query speedup

Key Insight: Memory had a greater impact than threading alone.

## Build Instructions
### Requirements
-  macOS / Linux  
-  C++17+  
-  CMake 3.16+  
-  OpenMP (libomp on macOS)  

### Build
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

### Running the Program
```bash
cd build/src
./mini1A
```

# Phase One Data
Per-run times (seconds)
Run 1: Ingest = 2498.09, Count = 7.91144, Execute = 7.46627

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 2.27023e+07 rows/sec
IO Throughput: 1732.05 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 2: Ingest = 2495.16, Count = 9.95065, Execute = 7.52258

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.80498e+07 rows/sec
IO Throughput: 1377.09 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 3: Ingest = 2495.94, Count = 7.96208, Execute = 7.39154

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 2.25579e+07 rows/sec
IO Throughput: 1721.03 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 4: Ingest = 2494.8, Count = 7.97953, Execute = 7.37052

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 2.25086e+07 rows/sec
IO Throughput: 1717.27 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 5: Ingest = 2493.63, Count = 9.78563, Execute = 7.42321

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.83542e+07 rows/sec
IO Throughput: 1400.32 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 6: Ingest = 2494.79, Count = 7.9208, Execute = 7.40929

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 2.26754e+07 rows/sec
IO Throughput: 1730 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 7: Ingest = 2519.12, Count = 6.82541, Execute = 7.60687

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 2.63146e+07 rows/sec
IO Throughput: 2007.64 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 8: Ingest = 2519.51, Count = 6.75285, Execute = 7.53919

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 2.65973e+07 rows/sec
IO Throughput: 2029.22 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 9: Ingest = 2525.62, Count = 6.87254, Execute = 7.55368

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 2.61341e+07 rows/sec
IO Throughput: 1993.88 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 10: Ingest = 2519.16, Count = 6.62485, Execute = 7.48575

Stats over 1 runs:
Ingest Time: Avg = 2519.16, Min = 2519.16, Max = 2519.16
Count Time: Avg = 6.62485, Min = 6.62485, Max = 6.62485
Execute Time: Avg = 7.48575, Min = 7.48575, Max = 7.48575

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 2.71112e+07 rows/sec
IO Throughput: 2068.42 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

# Phase 2 Data

Run 1: Ingest = 1225.57, Count = 10.7748, Execute = 6.6652

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.66693e+07 rows/sec
IO Throughput: 1271.76 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 2: Ingest = 1179.92, Count = 12.0216, Execute = 5.91896

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.49404e+07 rows/sec
IO Throughput: 1139.86 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 3: Ingest = 1178.62, Count = 10.0305, Execute = 4.37743

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.79062e+07 rows/sec
IO Throughput: 1366.13 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 4: Ingest = 1182.02, Count = 9.5559, Execute = 4.32447

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.87955e+07 rows/sec
IO Throughput: 1433.98 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 5: Ingest = 1280.78, Count = 10.059, Execute = 4.3714

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.78554e+07 rows/sec
IO Throughput: 1362.26 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 6: Ingest = 1192.53, Count = 10.7483, Execute = 4.42167

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.67104e+07 rows/sec
IO Throughput: 1274.9 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 7: Ingest = 1187.91, Count = 9.00934, Execute = 4.39155

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.99357e+07 rows/sec
IO Throughput: 1520.98 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 8: Ingest = 1193.31, Count = 11.5264, Execute = 4.26682

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.55823e+07 rows/sec
IO Throughput: 1188.83 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 9: Ingest = 1184.22, Count = 9.82774, Execute = 4.28775

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.82756e+07 rows/sec
IO Throughput: 1394.32 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

Run 10: Ingest = 1200.83, Count = 12.345, Execute = 4.27433

Derived Metrics:
Total Data Size: 13703 MiB
Row Throughput: 1.4549e+07 rows/sec
IO Throughput: 1110 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.026495 %

# Phase 3 Data
Run 1: Ingest = 1180.12, Count = 0.836962, Execute = 1.58062

Derived Metrics:
Total Data Size: 4453.47 MiB
Row Throughput: 2.14581e+08 rows/sec
IO Throughput: 5320.65 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.0264967 %

Run 2: Ingest = 1182.8, Count = 0.845597, Execute = 1.56403

Derived Metrics:
Total Data Size: 4453.47 MiB
Row Throughput: 2.1239e+08 rows/sec
IO Throughput: 5266.32 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.0264967 %

Run 3: Ingest = 1182.6, Count = 0.837278, Execute = 1.59034

Derived Metrics:
Total Data Size: 4453.47 MiB
Row Throughput: 2.145e+08 rows/sec
IO Throughput: 5318.65 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.0264967 %

Run 4: Ingest = 1184.99, Count = 0.837944, Execute = 1.56793

Derived Metrics:
Total Data Size: 4453.47 MiB
Row Throughput: 2.1433e+08 rows/sec
IO Throughput: 5314.42 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.0264967 %

Run 5: Ingest = 1177.95, Count = 0.844331, Execute = 1.56881

Derived Metrics:
Total Data Size: 4453.47 MiB
Row Throughput: 2.12708e+08 rows/sec
IO Throughput: 5274.22 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.0264967 %

Run 6: Ingest = 1183.03, Count = 0.844077, Execute = 1.56761

Derived Metrics:
Total Data Size: 4453.47 MiB
Row Throughput: 2.12772e+08 rows/sec
IO Throughput: 5275.8 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.0264967 %

Run 7: Ingest = 1181.46, Count = 0.846591, Execute = 1.59282

Derived Metrics:
Total Data Size: 4453.47 MiB
Row Throughput: 2.12141e+08 rows/sec
IO Throughput: 5260.14 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.0264967 %

Run 8: Ingest = 1185.86, Count = 0.851144, Execute = 1.57682

Derived Metrics:
Total Data Size: 4453.47 MiB
Row Throughput: 2.11006e+08 rows/sec
IO Throughput: 5232 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.0264967 %

Run 9: Ingest = 1184.58, Count = 0.849642, Execute = 1.57919

Derived Metrics:
Total Data Size: 4453.47 MiB
Row Throughput: 2.11379e+08 rows/sec
IO Throughput: 5241.25 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.0264967 %

Run 10: Ingest = 1177.08, Count = 0.838033, Execute = 1.56953

Derived Metrics:
Total Data Size: 4453.47 MiB
Row Throughput: 2.14307e+08 rows/sec
IO Throughput: 5313.85 MiB/sec
Parse Failure Rate: 0.00636944 %
Selectivity: 0.0264967 %
