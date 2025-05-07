#!/bin/bash

# Function to check if GEF is installed
check_gef() {
    if gdb -q -ex "pi import gef" -ex "q" 2>/dev/null | grep -q "gef"; then
        return 0  # GEF is installed
    else
        return 1  # GEF is not installed
    fi
}

# Wait for QEMU to start
pgrep -x "qemu-system-x86" > /dev/null
while [ $? -gt 0 ]; do
    sleep 1
    pgrep -x "qemu-system-x86" > /dev/null
done

# Ensure .gdb directory exists
if ! [ -d "./.gdb" ]; then
    mkdir ./.gdb
fi


# Generate the GDB script
cat > .gdb/.gdb_script.gdb << EOF
    file kernel/bin/kernel
    add-symbol-file userspace/bin/init
EOF

# If GEF is installed, add remote-safe GEF commands
if check_gef; then
    echo "    gef-remote localhost 1234" >> .gdb/.gdb_script.gdb
else
    echo "    target remote localhost:1234" >> .gdb/.gdb_script.gdb
fi

# If a breakpoint argument is provided, add it
if [ -e "$1" ]; then
    echo "    c" >> .gdb/.gdb_script.gdb
else
    echo "    break $1" >> .gdb/.gdb_script.gdb
    echo "    c" >> .gdb/.gdb_script.gdb
fi

# Run GDB with the generated script
if check_gef; then
    gdb -q -x .gdb/.gdb_script.gdb
else
    gdb -x .gdb/.gdb_script.gdb
fi
