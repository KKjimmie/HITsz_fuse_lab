#!/bin/bash

TEST_CASE="case 5 - remount"

function check_umount () {
    _PARAM=$1
    _TEST_CASE=$2
    
    sleep 1
    # sudo umount "${MNTPOINT}"
    umount "${MNTPOINT}"
    
    if ! check_mount; then
        return 0
    fi

    fail "$_TEST_CASE: $PROJECT_NAME文件系统仍然在挂载点${MNTPOINT}"
    return 1
}

ERR_OK=0
INODE_MAP_ERR=1
DATA_MAP_ERR=2
LAYOUT_FILE_ERR=3
GOLDEN_LAYOUT_MISMATCH=4

function check_bm() {
    _PARAM=$1
    _TEST_CASE=$2
    ROOT_PARENT_PATH=$(cd $(dirname $ROOT_PATH); pwd)
    python3 "$ROOT_PATH"/checkbm/checkbm.py -l "$ROOT_PARENT_PATH"/include/fs.layout -r "$ROOT_PARENT_PATH"/tests/checkbm/golden.json > /dev/null
    RET=$?
    if (( RET == ERR_OK )); then
        return 0
    elif (( RET == INODE_MAP_ERR )); then
        fail "$_TEST_CASE: Inode位图错误, 请使用checkbm.py和ddriver工具自行检查. 注: 在命令行输入ddriver -d并且安装HexEditor插件即可查看当前ddriver介质情况"
    elif (( RET == DATA_MAP_ERR )); then
        fail "$_TEST_CASE: 数据位图错误, 请使用checkbm.py和ddriver工具自行检查. 注: 在命令行输入ddriver -d并且安装HexEditor插件即可查看当前ddriver介质情况"
    elif (( RET == LAYOUT_FILE_ERR )); then
        fail "$_TEST_CASE: .layout文件有误, 请结合报错信息自行检查"
    elif (( RET == GOLDEN_LAYOUT_MISMATCH )); then
        fail "$_TEST_CASE: .layout文件与golden.json信息不一致, 请结合报错信息自行检查"
    fi
    return 1
}

clean_mount
clean_ddriver

try_mount_or_fail

mkdir_and_check "${MNTPOINT}/hello"

TEST_CASE="case 5.1 - umount ${MNTPOINT}"
core_tester ls "${MNTPOINT}" check_umount "$TEST_CASE"

sleep 1

TEST_CASE="case 5.2 - check bitmap"
core_tester ls "${MNTPOINT}" check_bm "$TEST_CASE" 15