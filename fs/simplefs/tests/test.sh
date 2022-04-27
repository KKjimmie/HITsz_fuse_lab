#!/bin/bash
echo "----测试阶段1：mount测试"
echo "----测试阶段2：mount 和 umount 测试"
echo "----测试阶段3：增加 mkdir 和 touch 测试"
echo "----测试阶段4：增加 ls 测试"
echo "----测试阶段5：增加 read 和 write 测试"
echo "----测试阶段6：增加 copy 测试"
read -r -p "按照你的进度输入测试等级[数字1-6]: " LEVEL 


if [[ "${LEVEL}" -ge "1" ]] && [[ "${LEVEL}" -le "6" ]]; then
    ./main.sh "${LEVEL}"
else
    echo "!! Wrong Test Level! Please input 1 to 6 !!"
fi


