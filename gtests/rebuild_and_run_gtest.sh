#!/bin/bash

mkdir -p build/log
#rm -rf ./build/*
find . -iwholename '*cpp/src' -not -iwholename '*lib/sqlitecpp/src' -type d -exec rm -r "{}" \;
mkdir -p build/CMakeFiles/testAll.dir
rm -r build/CMakeFiles/testAll.dir/
#mkdir -p build/log
./build_and_run_gtest.sh
