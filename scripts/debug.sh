#!/bin/bash

if [ -e "$1" ]; then
  echo "Usage: ./debug.sh [first_function_breakpoint]"
  exit 1
fi

pgrep -x "qemu-system-x86" >> /dev/null
while [ $? -gt 0 ]; do
  sleep 1
  pgrep -x "qemu-system-x86" >> /dev/null
done

if ! [ -d "./.gdb" ]; then
  mkdir ./.gdb
fi

cat > .gdb/.gdb_script.gdb << EOF
    file kernel/bin/kernel
    target remote localhost:1234
    break $1
    c
EOF

gdb -x .gdb/.gdb_script.gdb
