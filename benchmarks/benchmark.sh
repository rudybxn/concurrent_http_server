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

# --- Configuration -------------------------------------------
PORT=8080
THREADS=4
SCHEDULE=FCFS
DOCROOT=www/test

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

# --- Helper: start server ------------------------------------
start_server() {
    taskset -c 0,1 $SERVER $PORT $THREADS $SCHEDULE $DOCROOT &
    SERVER_PID=$!
    sleep 5  # wait for server to bind and start listening
}

# --- Helper: stop server -------------------------------------
stop_server() {
    kill $SERVER_PID
    wait $SERVER_PID 2>/dev/null || true
}

# --- Step 2: Run benchmark configurations --------------------
mkdir -p $RAW_DIR

for C in $CONCURRENCY_LEVELS; do
    for TRIAL in $(seq 1 $TRIALS); do
        WRK_THREADS=$(( C < 2 ? 1 : 2 ))

        # Uniform small
        echo "[bench] uniform small  | concurrency=$C | trial=$TRIAL"
        start_server
        taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$WARMUP_DURATION \
            $URL/small.bin > /dev/null
        taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$DURATION --latency \
            $URL/small.bin > $RAW_DIR/small_c${C}_t${TRIAL}.txt
        stop_server

        sleep 5

        # Uniform large
        echo "[bench] uniform large  | concurrency=$C | trial=$TRIAL"
        start_server
        taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$WARMUP_DURATION \
            $URL/large.bin > /dev/null
        taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$DURATION --latency \
            $URL/large.bin > $RAW_DIR/large_c${C}_t${TRIAL}.txt
        stop_server

        sleep 5

        # Heavy-tailed
        echo "[bench] heavy-tailed   | concurrency=$C | trial=$TRIAL"
        start_server
        taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$WARMUP_DURATION \
            -s $LUA_SCRIPT $URL > /dev/null
        taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$DURATION --latency \
            -s $LUA_SCRIPT $URL > $RAW_DIR/heavy_c${C}_t${TRIAL}.txt
        stop_server

        sleep 5

    done
done

echo ""
echo "Done. Raw results saved to $RAW_DIR/"

# --- Step 6: Parse raw results into CSV ----------------------
SUMMARY=$RAW_DIR/summary.csv
echo "workload,concurrency,trial,rps,mean_ms,p50_ms,p95_ms,p99_ms,errors" > $SUMMARY

parse_wrk() {
    local file=$1
    local workload=$2
    local concurrency=$3
    local trial=$4

    rps=$(grep "Requests/sec" "$file" | awk '{print $2}')
    mean=$(grep "Latency" "$file" | head -1 | awk '{print $2}' | sed 's/ms//')
    p50=$(grep "50%" "$file" | awk '{print $2}' | sed 's/ms//')
    p95=$(grep "95%" "$file" | awk '{print $2}' | sed 's/ms//')
    p99=$(grep "99%" "$file" | awk '{print $2}' | sed 's/ms//')
    errors=$(grep "Socket errors" "$file" | awk '{print $NF}' || echo 0)

    echo "$workload,$concurrency,$trial,$rps,$mean,$p50,$p95,$p99,$errors" >> $SUMMARY
}

for C in $CONCURRENCY_LEVELS; do
    for TRIAL in $(seq 1 $TRIALS); do
        parse_wrk $RAW_DIR/small_c${C}_t${TRIAL}.txt  "small"  $C $TRIAL
        parse_wrk $RAW_DIR/large_c${C}_t${TRIAL}.txt  "large"  $C $TRIAL
        parse_wrk $RAW_DIR/heavy_c${C}_t${TRIAL}.txt  "heavy"  $C $TRIAL
    done
done

echo "Summary written to $SUMMARY"