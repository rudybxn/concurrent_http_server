#!/bin/bash

# =============================================================
# clean.sh
# Removes all raw benchmark output files.
# =============================================================

SCRIPT_DIR=$(dirname "$0")
RAW_DIR=$SCRIPT_DIR/raw

if [ ! -d "$RAW_DIR" ]; then
    echo "Nothing to clean — $RAW_DIR does not exist."
    exit 0
fi

rm -f $RAW_DIR/*.txt $RAW_DIR/*.csv
echo "Cleaned $RAW_DIR/"