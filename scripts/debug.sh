#!/bin/bash

if [ -e "$1" ]; then

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
    c
EOF

else

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

fi

gdb -x .gdb/.gdb_script.gdb
