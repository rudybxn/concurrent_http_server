#!/bin/bash

# =============================================================
# benchmark.sh
# Prepares the environment, runs all benchmark configurations,
# and tears down the server when done.
# =============================================================

set -e  # exit immediately if any command fails

# --- Configuration -------------------------------------------
PORT=8080
THREADS=4
SCHEDULE=FCFS
DOCROOT=www/test

WRK_THREADS=2
DURATION=30s
CONCURRENCY_LEVELS="1 10 25 50 100 200"
RUNS=5

SERVER=./server
LUA_SCRIPT=heavy_tail.lua
RAW_DIR=raw
URL=http://localhost:$PORT

# --- Step 1: Minimize background noise -----------------------
echo "[setup] Stopping background services..."
sudo systemctl stop snapd 2>/dev/null || true
sudo systemctl stop unattended-upgrades 2>/dev/null || true

echo "[setup] Background services stopped."

# --- Step 2: Start the server pinned to cores 0-1 -----------
echo "[setup] Starting server on cores 0-1..."
taskset -c 0,1 $SERVER $PORT $THREADS $SCHEDULE $DOCROOT &
SERVER_PID=$!
echo "[setup] Server started (PID $SERVER_PID)."

# Give the server time to bind and start listening
sleep 2

# --- Step 3: Warmup ------------------------------------------
echo "[warmup] Running warmup (discarded)..."
taskset -c 2,3 wrk -t$WRK_THREADS -c10 -d5s $URL/small.bin > /dev/null
echo "[warmup] Done."

# --- Step 4: Run benchmark configurations --------------------
mkdir -p $RAW_DIR

for C in $CONCURRENCY_LEVELS; do
    for RUN in $(seq 1 $RUNS); do

        echo "[bench] uniform small  | concurrency=$C | run=$RUN"
        taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$DURATION --latency \
            $URL/small.bin > $RAW_DIR/small_c${C}_r${RUN}.txt

        sleep 5

        echo "[bench] uniform large  | concurrency=$C | run=$RUN"
        taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$DURATION --latency \
            $URL/large.bin > $RAW_DIR/large_c${C}_r${RUN}.txt

        sleep 5

        echo "[bench] heavy-tailed   | concurrency=$C | run=$RUN"
        taskset -c 2,3 wrk -t$WRK_THREADS -c$C -d$DURATION --latency \
            -s $LUA_SCRIPT $URL > $RAW_DIR/heavy_c${C}_r${RUN}.txt

        sleep 5

    done
done

# --- Step 5: Tear down ---------------------------------------
echo "[teardown] Killing server (PID $SERVER_PID)..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null || true
echo "[teardown] Server killed."

echo ""
echo "Done. Raw results saved to ./$RAW_DIR/"
