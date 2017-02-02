#!/bin/bash

mkdir -p build/log
cd build
cmake -Dtest=ON ..
make -j

echo "testAll"
./testAll
echo "in ./build/"

echo "simulator1"
./simulator1
echo "in ./build/"

echo "simulator2"
./simulator2
echo "in ./build/"

echo "simulator3"
./simulator3
echo "in ./build/"

echo "simulator4"
./simulator4
echo "in ./build/"
