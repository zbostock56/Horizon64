#!/bin/bash

find . -maxdepth 1 ! -name 'toolchain' ! -name '.git' -print0 | tar --null -czvf "horizon64_source_$(date +\%Y\%m\%d).tar.gz" --files-from=-

