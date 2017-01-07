#!/bin/bash

mkdir -p build/log
cd build
cmake -Dtest=ON ..
make -j

./testAll
echo "in ./build/"

./simulator1
echo "in ./build/"

./simulator2
echo "in ./build/"

./simulator3
echo "in ./build/"
