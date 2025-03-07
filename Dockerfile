FROM debian:latest

ARG CURRENT_DIR

RUN apt-get update -y
RUN apt-get install -y build-essential bison flex libgmp3-dev libmpc-dev \
                       libmpfr-dev texinfo wget nasm xorriso curl git \
                       qemu-system-x86 qemu-system-arm

COPY ./Makefile /opt

WORKDIR /opt
RUN make toolchain
WORKDIR ${CURRENT_DIR}

ENV CONTAINER=docker