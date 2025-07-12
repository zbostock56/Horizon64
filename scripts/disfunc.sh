#!/bin/bash

# Check if we received exactly one argument (the function name)
if [ $# -ne 1 ]; then
    echo "Usage: $0 <function_name>"
    exit 1
fi

if [ ! -f ./kernel/bin/kernel  ]; then
  echo "./kernel/bin/kernel doesn't exist."
  exit 1
fi

# Assign arguments to variables
binary_file=./kernel/bin/kernel
function_name=$1

# Get the function's start address from the symbol table using nm
start_address=$(nm -n $binary_file | grep " $function_name" | awk '{print $1}')

# Check if the function was found
if [ -z "$start_address" ]; then
    echo "Function '$function_name' not found in the symbol table."
    exit 1
fi

# Convert the start address from hexadecimal to decimal (objdump uses decimal addresses)
start_address_decimal=$((16#$start_address))

# Get the address of the next function to define the stop address
# This assumes the functions in the symbol table are listed in memory order.
next_function_address=$(nm -n $binary_file | grep -A 1 " $function_name" | tail -n 1 | awk '{print $1}')

# If the next function exists, calculate its decimal address, otherwise we assume the end of the binary
if [ -z "$next_function_address" ]; then
    # Set a stop address to the end of the binary
    stop_address_decimal=$(stat -c %s $binary_file)
else
    stop_address_decimal=$((16#$next_function_address))
fi

# Disassemble the function using objdump
objdump -d --start-address=$start_address_decimal --stop-address=$stop_address_decimal $binary_file
