#
#   Helper script to generate the system root folder
#   NOTE: This script is intended to be run in the project's root directory
#

if [ -d sysroot ]; then
    rm -rf sysroot
fi

mkdir -p sysroot

#
#   etc directory
#
mkdir -p sysroot/etc

#
#   root directory
#
mkdir -p sysroot/root
cat << 'EOF' > sysroot/root/bash_profile
HISTCONTROL=ignoredups
HISTSIZE=-1
HISTFILESIZE=-1
EOF