#!/bin/bash

# Get the current year
current_year=$(date +%Y)

# Directories to search
dirs=("kernel" "userspace" "h64libc")

# Loop through each directory
for dir in "${dirs[@]}"; do
    # Find all .c files in the directory
    find "$dir" -type f -name "*.c" | while read -r file; do
        # Check if the file contains a copyright line
        if grep -q "@copyright Copyright (c) [0-9]\{4\}" "$file"; then
            # Replace the copyright year with the current year
            sed -i "s/@copyright Copyright (c) [0-9]\{4\}/@copyright Copyright (c) $current_year/" "$file"
            echo "Updated: $file"
        fi
    done
done

