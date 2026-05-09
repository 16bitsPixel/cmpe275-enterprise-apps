#!/bin/bash
set -e

TARGET="127.0.0.1:50051"
CLIENT="./build/src/query_client"
MAX_ROWS=10
FETCHES=120

OUT_DIR="test_results"
RAW_FILE="$OUT_DIR/overlay_fetch_raw.txt"
SOURCES_FILE="$OUT_DIR/overlay_sources.txt"
SUMMARY_FILE="$OUT_DIR/overlay_summary.txt"

mkdir -p "$OUT_DIR"
: > "$RAW_FILE"
: > "$SOURCES_FILE"
: > "$SUMMARY_FILE"

echo "Submitting query to Node A..."
SUBMIT_OUTPUT=$($CLIENT --target "$TARGET" --submit --chunk-size "$MAX_ROWS")
echo "$SUBMIT_OUTPUT" | tee -a "$RAW_FILE"

REQUEST_ID=$(echo "$SUBMIT_OUTPUT" | awk '/request_id/ {print $3}')

if [ -z "$REQUEST_ID" ]; then
  echo "ERROR: Could not extract request_id"
  exit 1
fi

echo ""
echo "Using request_id=$REQUEST_ID"
echo "Fetching $FETCHES chunks from Node A..."
echo ""

for i in $(seq 1 "$FETCHES"); do
  echo "===== FETCH $i =====" | tee -a "$RAW_FILE"

  FETCH_OUTPUT=$($CLIENT --target "$TARGET" --fetch --request-id "$REQUEST_ID" --max-rows "$MAX_ROWS")
  echo "$FETCH_OUTPUT" >> "$RAW_FILE"

  NODE_ID=$(echo "$FETCH_OUTPUT" | awk '/node_id/ {print $3}')
  ROWS_RETURNED=$(echo "$FETCH_OUTPUT" | awk '/rows_returned/ {print $3}')
  DONE=$(echo "$FETCH_OUTPUT" | awk '/done/ {print $3}')

  echo "$FETCH_OUTPUT" | grep "source=" | sed -E 's/.*source=([^ ]+).*/\1/' >> "$SOURCES_FILE" || true

  CHUNK_SOURCES=$(echo "$FETCH_OUTPUT" | grep "source=" | sed -E 's/.*source=([^ ]+).*/\1/' | sort -u | tr '\n' ',' | sed 's/,$//')

  echo "fetch=$i node_id=$NODE_ID rows=$ROWS_RETURNED done=$DONE sources=$CHUNK_SOURCES"

  if [ "$DONE" = "true" ]; then
    echo "Query completed at fetch $i"
    break
  fi
done

UNIQUE_SOURCES=$(sort -u "$SOURCES_FILE" | tr '\n' ',' | sed 's/,$//')
SOURCE_COUNT=$(sort -u "$SOURCES_FILE" | wc -l | tr -d ' ')

has_source() {
  if grep -q "^$1$" "$SOURCES_FILE"; then
    echo "yes"
  else
    echo "no"
  fi
}

HAS_A=$(has_source A)
HAS_B=$(has_source B)
HAS_C=$(has_source C)
HAS_D=$(has_source D)
HAS_E=$(has_source E)
HAS_F=$(has_source F)
HAS_G=$(has_source G)
HAS_H=$(has_source H)
HAS_I=$(has_source I)

if [ "$HAS_F" = "yes" ]; then
  DEEPEST_PATH="confirmed: F -> E -> B -> A -> Client"
else
  DEEPEST_PATH="not confirmed in this run"
fi

{
  echo "===== OVERLAY COVERAGE SUMMARY ====="
  echo "request_id=$REQUEST_ID"
  echo "entry_node=A"
  echo "fetches_requested=$FETCHES"
  echo "max_rows_per_fetch=$MAX_ROWS"
  echo "unique_source_count=$SOURCE_COUNT"
  echo "sources_observed=$UNIQUE_SOURCES"
  echo "source_A=$HAS_A"
  echo "source_B=$HAS_B"
  echo "source_C=$HAS_C"
  echo "source_D=$HAS_D"
  echo "source_E=$HAS_E"
  echo "source_F=$HAS_F"
  echo "source_G=$HAS_G"
  echo "source_H=$HAS_H"
  echo "source_I=$HAS_I"
  echo "deepest_path=$DEEPEST_PATH"
  echo ""
  echo "Expected full coverage: A,B,C,D,E,F,G,H,I"
  echo "Expected deepest path: F -> E -> B -> A -> Client"
} | tee "$SUMMARY_FILE"

echo ""
echo "Saved raw output to: $RAW_FILE"
echo "Saved sources to: $SOURCES_FILE"
echo "Saved summary to: $SUMMARY_FILE"