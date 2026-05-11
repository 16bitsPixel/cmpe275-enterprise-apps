# Distributed Query Engine: NYC Yellow Taxi Trip Records

## Authors

Brandon Llanes  
Meghana Koti

---

## Overview

This project uses the NYC Yellow Taxi Trip Records dataset to explore query processing in two stages.

Mini 1 focuses on local query execution using streaming and in-memory query approaches.  
Mini 2 extends the project into a distributed multi-process query system using sharding, gRPC communication, and on-demand chunked result fetching.

---

## Dataset

Source: NYC OpenData – Yellow Taxi Trip Records  
Example: 2019 / 2020 Yellow Taxi Trip Data

The schema contains 18 columns including:

- Pickup / dropoff datetime
- Passenger count
- Trip distance
- Pickup/dropoff locations
- Payment type
- Fare, extra, tax, tip, and total amounts

---

## What the Project Does

### Mini 1: Streaming vs In-Memory Query Engine

#### 1) Ingestion

Reads one or more CSV files from a directory and parses each row into primitive values.  
Datetime strings are converted into **epoch milliseconds (UTC)** for fast comparisons in queries.

#### 2) Querying

Supports predicate-based queries over the dataset. Queries typically filter by:

- Pickup / dropoff time range
- Trip distance range
- Total amount range
- Tip amount range
- Payment type

Two query modes are supported:

- **count** – return number of matching rows
- **execute** – return the matching row set, references, or row indices depending on phase

#### 3) Benchmarking

Benchmarks compare streaming and in-memory query behavior across different query cases.

---

### Mini 2: Distributed Query Processing

Mini 2 extends the local query engine into a distributed multi-process system.

The main focus of Mini 2 is:

- Distributed data and queries
- Data sharding across multiple nodes
- gRPC-based communication between processes
- Request coordination through root node `A`
- Bounded result fetching using `SubmitQuery` and `FetchChunk`
- Comparing baseline precomputation with optimized on-demand processing

---

## Mini 2 Architecture

Mini 2 uses multiple server processes labeled `A` through `I`.

Node `A` acts as the root coordinator and is the only node that communicates directly with the client. Other nodes own their assigned data shards and participate in distributed query execution.

At a high level:

1. The client submits a query to node `A`.
2. Node `A` creates a request context for the query.
3. Node `A` forwards the query through the configured process network.
4. Each node evaluates the query against its own local shard.
5. Matching results are gathered back toward node `A`.
6. The client fetches results in bounded chunks using the returned `request_id`.

```text
[Client]
   |
   v
[Node A - Root Coordinator]
   |
   +--------------------+
   |                    |
[Child Nodes]       [Child Nodes]
   |                    |
[Local Shards]      [Local Shards]
```

---

## Mini 2 Data Sharding

The dataset is split across nodes `A` through `I`.

Each node owns only one shard of the dataset, so the full dataset is not scanned by a single process. This supports distributed query execution and helps compare how shard assignment affects workload distribution.

Shard directory structure:

```text
data/
  shards/
    A/
    B/
    C/
    D/
    E/
    F/
    G/
    H/
    I/
```

NOTE: Replace `<CSV_FILE>` with the correct input CSV file path.

```sh
mkdir -p data/shards/{A,B,C,D,E,F,G,H,I}

for n in A B C D E F G H I; do
  head -n 1 data/<CSV_FILE> > data/shards/$n/yellow_2020_$n.csv
done

tail -n +2 data/<CSV_FILE> | awk '
{
  shard = NR % 9
  if (shard == 1) print >> "data/shards/A/yellow_2020_A.csv"
  else if (shard == 2) print >> "data/shards/B/yellow_2020_B.csv"
  else if (shard == 3) print >> "data/shards/C/yellow_2020_C.csv"
  else if (shard == 4) print >> "data/shards/D/yellow_2020_D.csv"
  else if (shard == 5) print >> "data/shards/E/yellow_2020_E.csv"
  else if (shard == 6) print >> "data/shards/F/yellow_2020_F.csv"
  else if (shard == 7) print >> "data/shards/G/yellow_2020_G.csv"
  else if (shard == 8) print >> "data/shards/H/yellow_2020_H.csv"
  else print >> "data/shards/I/yellow_2020_I.csv"
}'
```

---

## Mini 2 Query Flow

### 1) Submit Query

The client sends a query request to node `A`.

```sh
./src/query_client --target 127.0.0.1:50051 --submit --max-rows 10
```

The submit call returns a `request_id`.

### 2) Fetch Results

The client uses the returned `request_id` to fetch results in chunks.

```sh
./src/query_client --target 127.0.0.1:50051 --fetch --request-id <request_id> --max-rows 200
```

This allows large result sets to be retrieved gradually instead of being returned in one large response.

---

## Baseline vs Optimized Processing

Mini 2 compares two processing strategies.

### Baseline

The baseline performs more work upfront during `SubmitQuery`.
This makes later fetches cheaper because results have already been precomputed.

### Optimized

The optimized version avoids one large upfront processing step.
Instead, it processes data on demand in bounded chunks during fetch operations.

This comparison helped evaluate:

- How shard assignment affects load distribution
- How precomputation compares with on-demand processing
- How client-observed query time changes across query cases
- How distributed request handling can support fairness and scalability

Benchmark chart:

```text
test_results/query_latency_baseline_vs_optimized.png
```

---

## Build

Use the provided Mini 2 build script from the project root folder.

```sh
chmod +x build_mini2.sh
./build_mini2.sh
```

The script handles the Mini 2 build setup and generates the required build output.

After a successful build, the executables should be available under the build directory:

```text
build/src/node_server
build/src/query_client
```

---

## Run Mini 2

Start each server node in a separate terminal.

NOTE: Start nodes from leaf to root. Start downstream nodes first and start node `A` last.

```sh
cd build
./src/node_server --config ../basic/config/<conf_file>
```

Example:

```sh
cd build
./src/node_server --config ../basic/config/node_A.conf
```

All client queries are sent to root node `A`.

Submit query:

```sh
./src/query_client --target 127.0.0.1:50051 --submit --max-rows 10
```

Fetch query results:

```sh
./src/query_client --target 127.0.0.1:50051 --fetch --request-id <request_id> --max-rows 200
```

---

## Configuration

Configuration files define each node's identity, host, port, shard path, and neighbor connections.

Server identity, hostnames, and edges should not be hardcoded in the source code.

Example config location:

```text
basic/config/
```

If using templates:

```text
basic/config_templates/
```

---

## Notes

- Node `A` is the only node that replies directly to the client.
- Each server process owns a local shard of the dataset.
- gRPC is used for communication between distributed processes.
- Results are fetched using bounded chunks instead of one large response.
- The optimized design focuses on on-demand chunking and fairer distributed request handling.
