#!/bin/bash

# ["cd", "./fs/fs"],
# ["make", "clean"],
# ["make"],
# ["cd", "./tests"],
# ["bash", "./test.sh"],
# ["E"]

cd ./fs/testfs || exit
rm build -rf
mkdir build && cd build && cmake .. && make
cd ../tests || exit
echo E | ./test.sh