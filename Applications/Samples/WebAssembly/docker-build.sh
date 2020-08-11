#!/bin/bash

set -ex

IMAGE=jodogne/wasm-builder:1.39.17-upstream

if [ "$1" != "Debug" -a "$1" != "Release" ]; then
    echo "Please provide build type: Debug or Release"
    exit -1
fi

if [ -t 1 ]; then
    # TTY is available => use interactive mode
    DOCKER_FLAGS='-i'
fi

ROOT_DIR=`dirname $(readlink -f $0)`/../../..

mkdir -p ${ROOT_DIR}/wasm-binaries

docker run -t ${DOCKER_FLAGS} --rm \
    --user $(id -u):$(id -g) \
    -v ${ROOT_DIR}:/source:ro \
    -v ${ROOT_DIR}/wasm-binaries:/target:rw ${IMAGE} \
    bash /source/Applications/Samples/WebAssembly/docker-internal.sh $1

ls -lR ${ROOT_DIR}/wasm-binaries/
