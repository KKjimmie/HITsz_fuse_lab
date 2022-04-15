#!/bin/bash

read -r -p "请输入测试等级[N(普通)/E(进阶)]: " LEVEL 

if [[ "${LEVEL}" == "E" ]]; then
    ./main.sh "E"
else
    ./main.sh "N"
fi


