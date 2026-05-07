# NYC Yellow Taxi Trip Records – Streaming vs In-Memory Query Engine (Phases 1–3)

## Overview

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

## Data Sharding
NOTE: replace <CSV_FILE> with the correct path
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

## Build
```sh
rm -rf build
mkdir build
cd build

unset CPATH
unset CPLUS_INCLUDE_PATH
unset C_INCLUDE_PATH
unset LIBRARY_PATH
unset LD_LIBRARY_PATH
unset DYLD_LIBRARY_PATH
unset CMAKE_PREFIX_PATH
unset CXXFLAGS
unset LDFLAGS

CC=/usr/bin/clang CXX=/usr/bin/clang++ cmake ../basic \
  -DCMAKE_PREFIX_PATH="/opt/homebrew" \
  -DProtobuf_DIR="/opt/homebrew/lib/cmake/protobuf" \
  -DgRPC_DIR="/opt/homebrew/lib/cmake/grpc"

make -j
```

## Run
Start a server node (A-I). <br>
NOTE: start nodes from I to A (leaf to root)
```sh
./src/node_server --config ../basic/config/<conf_file>
```

Query (all queries sent to root A)
```sh
./src/query_client --target 127.0.0.1:50051 --submit --max-rows 10
```

Fetch. <br>
<request_id> is returned from the previous query call
```sh
./src/query_client --target 127.0.0.1:50051 --fetch --request-id <request_id> --max-rows 200
```