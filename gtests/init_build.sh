#!/bin/bash

cd ..
git submodule init
git submodule update
cd lib/g3log
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DCHANGE_G3LOG_DEBUG_TO_DBUG=On ..
make -j
cd ../../..
cd gtests
