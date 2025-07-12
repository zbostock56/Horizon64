#!/bin/bash

docker build --build-arg CURRENT_DIR="${PWD}" -t ghcr.io/zbostock56/cross-compiler:0.5 .
