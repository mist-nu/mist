#!/bin/bash

mkdir -p build/log
cd build
cmake -Dtest=ON ..
make -j
./testAll
echo "in ./build/"
