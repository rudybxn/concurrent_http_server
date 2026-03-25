#!/bin/bash

# =============================================================
# verify.sh
# Checks that all expected benchmark output files exist and
# are non-empty. Prints a summary of any missing or empty files.
# =============================================================

SCRIPT_DIR=$(dirname "$0")
RAW_DIR=$SCRIPT_DIR/raw

CONCURRENCY_LEVELS="10 25 50 100 200"
WORKLOADS="small large heavy"
TRIALS=5

MISSING=0
EMPTY=0
ERRORS=()

for WORKLOAD in $WORKLOADS; do
    for C in $CONCURRENCY_LEVELS; do
        for TRIAL in $(seq 1 $TRIALS); do
            FILE=$RAW_DIR/${WORKLOAD}_c${C}_t${TRIAL}.txt

            if [ ! -f "$FILE" ]; then
                ERRORS+=("MISSING: $FILE")
                MISSING=$((MISSING + 1))
            elif [ ! -s "$FILE" ]; then
                ERRORS+=("EMPTY:   $FILE")
                EMPTY=$((EMPTY + 1))
            fi
        done
    done
done

EXPECTED=$((5 * 5 * 3))  # concurrency levels * trials * workloads
FOUND=$(ls $RAW_DIR/*.txt 2>/dev/null | wc -l)

echo "================================================"
echo " Benchmark Verification"
echo "================================================"
echo " Expected files : $EXPECTED"
echo " Found files    : $FOUND"
echo " Missing        : $MISSING"
echo " Empty          : $EMPTY"
echo "------------------------------------------------"

if [ ${#ERRORS[@]} -eq 0 ]; then
    echo " All $EXPECTED configurations verified successfully."
else
    echo " Issues found:"
    for ERR in "${ERRORS[@]}"; do
        echo "   $ERR"
    done
    echo ""
    echo " Rerun ./benchmarks/benchmark.sh to collect missing data,"
    echo " or run ./benchmarks/clean.sh first for a full reset."
fi

echo "================================================"