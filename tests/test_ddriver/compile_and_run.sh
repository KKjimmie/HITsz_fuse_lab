#!/bin/bash

mkdir build

cd build || exit

cmake ..

make

./ddriver_test

cd - || exit