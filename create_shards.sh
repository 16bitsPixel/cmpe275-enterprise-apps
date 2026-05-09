#!/bin/bash
set -e

cd basic

rm -rf data/shards
mkdir -p data/shards/{A,B,C,D,E,F,G,H,I}

copy_shard() {
  node="$1"
  src="$2"
  dst="data/shards/$node/yellow_2020_$node.csv"

  echo "Assigning $src -> shard $node"

  if [ ! -f "$src" ]; then
    echo "ERROR: missing file $src"
    exit 1
  fi

  cp "$src" "$dst"
}

copy_shard A "data/subset/yellow_tripdata_2020-01.csv"
copy_shard B "data/subset/yellow_tripdata_2020-02.csv"
copy_shard C "data/subset/yellow_tripdata_2020-03.csv"
copy_shard D "data/subset/yellow_tripdata_2020-04.csv"
copy_shard E "data/subset/yellow_tripdata_2020-05.csv"
copy_shard F "data/subset/yellow_tripdata_2020-06.csv"
copy_shard G "data/subset/yellow_tripdata_2020-07.csv"
copy_shard H "data/subset/yellow_tripdata_2020-08.csv"
copy_shard I "data/subset/yellow_tripdata_2020-09.csv"

echo "Done creating 9 node-local shards."
ls -lh data/shards/*/*.csv
wc -l data/shards/*/*.csv