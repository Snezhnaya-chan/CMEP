#!/bin/bash

current_directory=$(pwd)

shopt -s globstar

./build.sh
cmake --build . --target runtest

lcov --directory . --zerocounters

cd build

./runtest all

cd ..

lcov --no-external --directory . --capture --output-file app_total.info 

lcov --remove app_total.info -o app_filtered.info \
    ${current_directory}/EngineCore/include/glm/*/* \
    ${current_directory}/EngineCore/include/Rendering/tinyobjloader/* \
    ${current_directory}/external/**

genhtml --branch-coverage app_filtered.info