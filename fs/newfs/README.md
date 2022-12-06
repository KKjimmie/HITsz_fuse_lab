# HITsz-OS-lab #5
> Fuse-based ext2 file system

## Passed Tests
- [x] mount
- [x] mkdir, touch
- [x] ls
- [x] umount, remount
- [x] read, write
- [ ] copy
- [ ] ...
---
- [x] 数据块动态分配
- [x] 只写回被修改的数据块
- [ ] ...

--- 
- 挂载文件系统：
配置好`.vscode/task.json`文件后，在 vscode 中按下 **F5** 编译运行，挂载文件系统。

- 卸载文件系统：执行如下命令：`fusermount -u {mntpath}`
`{mntpath}`是挂载点路径，默认为 `./tests/mnt`。