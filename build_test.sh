#!/bin/bash
set -e
rm -rf gtest/build
cmake -DCMAKE_BUILD_TYPE=Release -S gtest -B gtest/build
cmake --build gtest/build
ctest --test-dir gtest/build -C Release --output-on-failure --rerun-failed
