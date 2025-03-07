#!/bin/bash

# Exit script on error
set -e

# Function to find the 'kernel' directory up to two levels up
find_kernel_dir() {
    local current_dir=$(pwd)
    
    for level in . .. ../..; do
        if [ -d "$level/kernel" ]; then
            echo "$(realpath "$level/kernel")"
            return
        fi
    done

    echo "Error: 'kernel' directory not found up to two levels up." >&2
    exit 1
}

# Find the 'kernel' directory
KERNEL_DIR=$(find_kernel_dir)

# Run cppcheck on the 'kernel' directory
echo "Running cppcheck on directory: $KERNEL_DIR"

cppcheck --enable=all \
         --std=c11 \
         --max-ctu-depth=16 \
         -j4 \
         --inconclusive \
         --force \
         "$KERNEL_DIR"

