#!/bin/bash

# =============================================================
# benchmark.sh
# Prepares the environment, runs all benchmark configurations,
# and tears down the server when done.
#
# Protocol:
#   - 5 independent trials per configuration
#   - 5-second warmup per trial (excluded from measurement)
#   - Server restarted between trials to clear warm state
# =============================================================

set -e

# --- Argument parsing ----------------------------------------
WORKLOAD_FILTER=""

while getopts "w:" opt; do
    case $opt in
        w) WORKLOAD_FILTER=$OPTARG ;;
        *) echo "Usage: $0 [-w small|large|heavy]"; exit 1 ;;
    esac
done

if [[ -n "$WORKLOAD_FILTER" && "$WORKLOAD_FILTER" != "small" && \
      "$WORKLOAD_FILTER" != "large" && "$WORKLOAD_FILTER" != "heavy" ]]; then
    echo "Error: -w must be one of: small, large, heavy"
    exit 1
fi

# --- Configuration -------------------------------------------
PORT=8080
THREADS=4
SCHEDULE=FCFS

WARMUP_DURATION=5s
DURATION=30s
CONCURRENCY_LEVELS="1 10 25 50 100 200"
TRIALS=5

SERVER=./server
SCRIPT_DIR=$(dirname "$0")
LUA_SCRIPT=$SCRIPT_DIR/heavy_tail.lua
RAW_DIR=$SCRIPT_DIR/raw
URL=http://localhost:$PORT

# --- Step 1: Minimize background noise -----------------------
echo "[setup] Stopping background services..."
sudo systemctl stop snapd 2>/dev/null || true
sudo systemctl stop unattended-upgrades 2>/dev/null || true
echo "[setup] Background services stopped."
echo ""
echo "======= BENCHMARK START ======="

# --- Helper: start server ------------------------------------
start_server() {
    taskset -c 0,1 $SERVER -p $PORT -t $THREADS -b 16 -s $SCHEDULE > $RAW_DIR/server.log 2>&1 &
    SERVER_PID=$!

    # Poll until server is accepting connections
    for i in $(seq 1 15); do
        sleep 1
        if nc -z localhost $PORT 2>/dev/null; then
            echo "[setup] Server ready to accept connections (PID $SERVER_PID)."
            return
        fi
        echo "[setup] Waiting for server... ($i/15)"
    done

    echo "[error] Server failed to start after 15 seconds. Aborting."
    exit 1
}

# --- Helper: stop server -------------------------------------
stop_server() {
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null || true
    # Wait for port to be fully released
    while nc -z localhost $PORT 2>/dev/null; do
        echo "[teardown] Waiting for port to be released..."
        sleep 1
    done
    echo "===== Server Stopped ====="
    echo ""
}

# --- Step 2: Run benchmark configurations --------------------
mkdir -p $RAW_DIR

for C in $CONCURRENCY_LEVELS; do
    for TRIAL in $(seq 1 $TRIALS); do
        WRK_THREADS=$(( C < 2 ? 1 : 2 ))
        echo ""

        # Uniform small
        if [[ -z "$WORKLOAD_FILTER" || "$WORKLOAD_FILTER" == "small" ]]; then
            echo "[BENCHMARK] workload=uniform small  | concurrency=$C | trial=$TRIAL"
            start_server
            taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$WARMUP_DURATION \
                $URL/test/small.bin > /dev/null || true
            taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$DURATION --latency \
                $URL/test/small.bin > $RAW_DIR/small_c${C}_t${TRIAL}.txt || true
            stop_server
            sleep 8
        fi

        # Uniform large
        if [[ -z "$WORKLOAD_FILTER" || "$WORKLOAD_FILTER" == "large" ]]; then
            echo "[BENCHMARK] workload=uniform large  | concurrency=$C | trial=$TRIAL"
            start_server
            taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$WARMUP_DURATION \
                $URL/test/large.bin > /dev/null || true
            taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$DURATION --latency \
                $URL/test/large.bin > $RAW_DIR/large_c${C}_t${TRIAL}.txt || true
            stop_server
            sleep 8
        fi

        # Heavy-tailed
        if [[ -z "$WORKLOAD_FILTER" || "$WORKLOAD_FILTER" == "heavy" ]]; then
            echo "[BENCHMARK] workload=heavy-tailed   | concurrency=$C | trial=$TRIAL"
            start_server
            taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$WARMUP_DURATION \
                -s $LUA_SCRIPT $URL > /dev/null || true
            taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$DURATION --latency \
                -s $LUA_SCRIPT $URL > $RAW_DIR/heavy_c${C}_t${TRIAL}.txt || true
            stop_server
            sleep 8
        fi

    done
done

# --- Step 3: Tear down ---------------------------------------
echo ""
echo "Done. Raw results saved to $RAW_DIR/"

# --- Step 4: Parse raw results into CSV ----------------------
SUMMARY=$RAW_DIR/summary.csv
echo "workload,concurrency,trial,rps,mean_ms,p50_ms,p75_ms,p90_ms,p99_ms,errors" > $SUMMARY

parse_wrk() {
    local file=$1
    local workload=$2
    local concurrency=$3
    local trial=$4

    [ -f "$file" ] || return
    rps=$(grep "Requests/sec" "$file" | awk '{print $2}')
    mean=$(grep "Latency" "$file" | head -1 | awk '{print $2}' | sed 's/ms//')
    p50=$(grep "50%" "$file" | awk '{print $2}' | sed 's/ms//')
    p75=$(grep "75%" "$file" | awk '{print $2}' | sed 's/ms//')
    p90=$(grep "90%" "$file" | awk '{print $2}' | sed 's/ms//')
    p99=$(grep "99%" "$file" | awk '{print $2}' | sed 's/ms//')
    errors=$(grep "Socket errors" "$file" | awk '{print $NF}' || echo 0)

    echo "$workload,$concurrency,$trial,$rps,$mean,$p50,$p75,$p90,$p99,$errors" >> $SUMMARY
}

for C in $CONCURRENCY_LEVELS; do
    for TRIAL in $(seq 1 $TRIALS); do
        parse_wrk $RAW_DIR/small_c${C}_t${TRIAL}.txt  "small"  $C $TRIAL
        parse_wrk $RAW_DIR/large_c${C}_t${TRIAL}.txt  "large"  $C $TRIAL
        parse_wrk $RAW_DIR/heavy_c${C}_t${TRIAL}.txt  "heavy"  $C $TRIAL
    done
done

echo "Summary written to $SUMMARY"