#include "nfs.h"

extern struct nfs_super nfs_super;                   // 内存超级块
extern struct custom_options nfs_options;			 /* 全局选项 */

/**
 * @brief 获取文件名
 * 
 * @param path 
 * @return char* 
 */
char* nfs_get_fname(const char* path) {
    char ch = '/';
    char *q = strrchr(path, ch) + 1;
    return q;
}

/**
 * @brief 计算路径的层级
 * exm: /av/c/d/f
 * -> lvl = 4
 * @param path 
 * @return int 
 */
int nfs_calc_lvl(const char * path) {
    // char* path_cpy = (char *)malloc(strlen(path));
    // strcpy(path_cpy, path);
    const char* str = path;
    int   lvl = 0;
    if (strcmp(path, "/") == 0) {
        return lvl;
    }
    while (*str != '\0') {
        if (*str == '/') {
            lvl++;
        }
        str++;
    }
    return lvl;
}

/**
 * @brief 创建目录项
 * 
 * @param fname 目录项文件名
 * @param ftype 文件类型
 * @return 目录项指针
*/
struct nfs_dentry *new_dentry(char* fname, NFS_FILE_TYPE ftype) {
	struct nfs_dentry *dentry = (struct nfs_dentry *)malloc(sizeof(struct nfs_dentry));
	memset(dentry, 0, sizeof(struct nfs_dentry));
	NFS_ASSIGN_FNAME(dentry, fname);
	dentry->ftype = ftype;
	dentry->ino = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL;   
	dentry->valid = 1;
	return dentry;
}


/**
 * @brief 驱动读，按照一个块大小1024B
 * 
 * @param offset 偏移
 * @param out_content 读出内容 
 * @param size 
 * @return int 
 */
int nfs_driver_read(int offset, uint8_t *out_content, int size) {
    // offset 向下取整
    int      offset_aligned = NFS_ROUND_DOWN(offset, NFS_BLK_SZ());
    // memcpy 用
    int      bias           = offset - offset_aligned;
    // size 向上取整
    int      size_aligned   = NFS_ROUND_UP((size + bias), NFS_BLK_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // lseek(NFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(NFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // read(NFS_DRIVER(), cur, NFS_IO_SZ());
        // ddriver 一次读 512B
        ddriver_read(NFS_DRIVER(), (char *)cur, NFS_IO_SZ());
        cur          += NFS_IO_SZ();
        size_aligned -= NFS_IO_SZ();   
    }
    memcpy(out_content, temp_content + bias, size);
    free(temp_content);
    return NFS_ERROR_NONE;
}


/**
 * @brief 驱动写
 * 
 * @param offset 偏移
 * @param in_content 写入内容
 * @param size 
 * @return int 
 */
int nfs_driver_write(int offset, uint8_t *in_content, int size) {
    int      offset_aligned = NFS_ROUND_DOWN(offset, NFS_BLK_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = NFS_ROUND_UP((size + bias), NFS_BLK_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // 每次写进 disk 512B,先读出原有内容
    nfs_driver_read(offset_aligned, temp_content, size_aligned);
    memcpy(temp_content + bias, in_content, size);
    
    // lseek(NFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(NFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        ddriver_write(NFS_DRIVER(), (char *)cur, NFS_IO_SZ());
        cur          += NFS_IO_SZ();
        size_aligned -= NFS_IO_SZ();   
    }
    free(temp_content);
    return NFS_ERROR_NONE;
}

/**
 * @brief 分配一个inode，占用位图
 * 
 * @param dentry 该dentry指向分配的inode
 * @return nfs_inode
 */
struct nfs_inode* nfs_alloc_inode(struct nfs_dentry * dentry) {
    struct nfs_inode* inode;
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int ino_cursor  = 0;
    int is_find_free_entry = 0;

    // 从 inode 位图分配
    for (byte_cursor = 0; byte_cursor < NFS_BLKS_SZ(nfs_super.map_inode_blks); 
         byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((nfs_super.map_inode[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前ino_cursor位置空闲 */
                nfs_super.map_inode[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = 1;           
                break;
            }
            ino_cursor++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    if (!is_find_free_entry || ino_cursor == nfs_super.max_ino) {
        return -NFS_ERROR_NOSPACE;
    }

    inode = (struct nfs_inode*)malloc(sizeof(struct nfs_inode));
    inode->ino  = ino_cursor; 
    printf("ino_cursor :%d", ino_cursor);
    inode->size = 0;
                                                      /* dentry指向inode */
    dentry->inode = inode;
    dentry->ino   = inode->ino;
                                                      /* inode指回dentry */
    inode->dentry = dentry;
    
    inode->dir_cnt = 0;
    inode->dentrys = NULL;
    
    // 从 data bitmap 分配
    for(int i = 0; i < NFS_DATA_PER_FILE; i ++) {
        inode->block_pointer[i] = nfs_alloc_data();
    }

    if (NFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(NFS_BLKS_SZ(NFS_DATA_PER_FILE));
    }

    return inode;
}

/**
 * @brief 分配一个数据块
 * @return 返回 data_cursor
*/
int nfs_alloc_data(){
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int data_cursor  = 0;
    int is_find_free_data = 0;

    for (byte_cursor = 0; byte_cursor < nfs_super.data_blks; 
         byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((nfs_super.map_data[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前data_cursor位置空闲 */
                nfs_super.map_data[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_data = 1;           
                break;
            }
            data_cursor++;
        }
        if (is_find_free_data) {
            break;
        }
    }
    if (!is_find_free_data|| data_cursor >= nfs_super.data_blks) {
        return -NFS_ERROR_NOSPACE;
    }
    return data_cursor;
}

/**
 * @brief 将内存inode及其下方结构全部刷回磁盘
 * 
 * @param inode 
 * @return int 
 */
int nfs_sync_inode(struct nfs_inode * inode) {
    struct nfs_inode_d  inode_d;
    struct nfs_dentry*  dentry_cursor;
    struct nfs_dentry_d dentry_d;
    uint8_t *tmp_data;
    int blk_no          = 0;
    int ino             = inode->ino;
    inode_d.ino         = ino;
    inode_d.size        = inode->size;
    inode_d.ftype       = inode->dentry->ftype;
    inode_d.dir_cnt     = inode->dir_cnt;
    int offset;

    for (int i = 0; i < NFS_DATA_PER_FILE; ++i) {
        inode_d.block_pointer[i] = inode->block_pointer[i];
    }

    if (nfs_driver_write(NFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                     sizeof(struct nfs_inode_d)) != 0) {
        return -NFS_ERROR_IO;
    }
                                                      /* Cycle 1: 写 INODE */
                                                      /* Cycle 2: 写 数据 */
    if (NFS_IS_DIR(inode)) {             
        // 如果是目录类型
        // 将目录项存到 data block 中
        dentry_cursor = inode->dentrys;
        while (dentry_cursor != NULL && blk_no < NFS_DATA_PER_FILE) {
            offset = NFS_DATA_OFS(inode_d.block_pointer[blk_no]);
            while (dentry_cursor != NULL)
            {
                memcpy(dentry_d.fname, dentry_cursor->fname, MAX_NAME_LEN);
                dentry_d.ftype = dentry_cursor->ftype;
                dentry_d.ino = dentry_cursor->ino;
                if (nfs_driver_write(offset, (uint8_t *)&dentry_d, 
                                    sizeof(struct nfs_dentry_d)) != 0) {
                    return -NFS_ERROR_IO;                     
                }
                
                if (dentry_cursor->inode != NULL) {
                    nfs_sync_inode(dentry_cursor->inode);
                }

                dentry_cursor = dentry_cursor->brother;
                offset += sizeof(struct nfs_dentry_d);
                if (offset >= NFS_DATA_OFS(inode_d.block_pointer[blk_no]) + 1){
                    break;
                }
            }
            blk_no ++;
        }
    }
    else if (NFS_IS_REG(inode)) {
        // 如果是文件类型，一块一块地写入
        tmp_data = inode->data;
        for (int i = 0; i < NFS_DATA_PER_FILE; ++i) {
            if (nfs_driver_write(NFS_DATA_OFS(inode_d.block_pointer[i]), tmp_data, 
                                NFS_BLK_SZ()) != 0) {
                return -NFS_ERROR_IO;
            }
            tmp_data += NFS_BLK_SZ();
        }
    }
    return NFS_ERROR_NONE;
}
/**
 * @brief 为一个inode分配dentry，采用头插法
 * 
 * @param inode 
 * @param dentry 
 * @return int 
 */
int nfs_alloc_dentry(struct nfs_inode* inode, struct nfs_dentry* dentry) {
    if (inode->dentrys == NULL) {
        inode->dentrys = dentry;
    }
    else {
        dentry->brother = inode->dentrys;
        inode->dentrys = dentry;
    }
    inode->dir_cnt++;
    return inode->dir_cnt;
}
/**
 * @brief 
 * 
 * @param dentry dentry指向ino，读取该inode
 * @param ino inode唯一编号
 * @return struct nfs_inode* 
 */
struct nfs_inode* nfs_read_inode(struct nfs_dentry * dentry, int ino) {
    struct nfs_inode* inode = (struct nfs_inode*)malloc(sizeof(struct nfs_inode));
    struct nfs_inode_d inode_d;
    struct nfs_dentry* sub_dentry;
    struct nfs_dentry_d dentry_d;
    int    dir_cnt = 0, i;
    if (nfs_driver_read(NFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                        sizeof(struct nfs_inode_d)) != 0) {
        NFS_DBG("[%s] io error\n", __func__);
        return NULL;                    
    }
    inode->dir_cnt = 0;
    inode->ino = inode_d.ino;
    inode->size = inode_d.size;
    inode->dentry = dentry;
    inode->dentrys = NULL;
    if (NFS_IS_DIR(inode)) {
        dir_cnt = inode_d.dir_cnt;
        for (i = 0; i < dir_cnt; i++)
        {
            if (nfs_driver_read(NFS_DATA_OFS(inode_d.block_pointer[0]) + i * sizeof(struct nfs_dentry_d), 
                                (uint8_t *)&dentry_d, 
                                sizeof(struct nfs_dentry_d)) != 0) {
                return NULL;                    
            }
            sub_dentry = new_dentry(dentry_d.fname, dentry_d.ftype);
            sub_dentry->parent = inode->dentry;
            sub_dentry->ino    = dentry_d.ino; 
            nfs_alloc_dentry(inode, sub_dentry);
        }
    }
    else if (NFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(NFS_BLKS_SZ(NFS_DATA_PER_FILE));
        uint8_t *tmp_data = inode->data;
        for (int i = 0; i < NFS_DATA_PER_FILE; ++i) {
            if (nfs_driver_read(NFS_DATA_OFS(inode_d.block_pointer[i]), (uint8_t *)tmp_data, 
                            NFS_BLK_SZ()) != 0) {
                NFS_DBG("[%s] io error\n", __func__);
                return NULL;
            }
            tmp_data += NFS_BLK_SZ();
        }
    }
    return inode;
}

/**
 * @brief 挂载sfs, Layout 如下
 * 
 * Layout
 * | Super(1) | Inode Map(1) | Data Map(1) | Inode | Data
 * 
 * BLK_SZ = 2*IO_SZ
 * 
 * 每个 Inode 占用一个Blk
 * @param options 
 * @return int: 0 success, else error
 */
int nfs_mount(struct custom_options options){
    int driver_fd;
    struct nfs_super_d nfs_super_d;
    struct nfs_dentry *root_dentry;
    struct nfs_inode* root_inode;

    int inode_num;
    int map_inode_blks;
    int map_data_blks;

    int super_blks;
    int is_init = 0;

    nfs_super.is_mounted = 0;

    driver_fd = ddriver_open(options.device);

    if (driver_fd < 0) {
        return driver_fd;
    }

    nfs_super.driver_fd = driver_fd;
    ddriver_ioctl(NFS_DRIVER(), IOC_REQ_DEVICE_SIZE, &nfs_super.sz_disk);
    ddriver_ioctl(NFS_DRIVER(), IOC_REQ_DEVICE_IO_SZ, &nfs_super.sz_io);
    nfs_super.sz_block = 1024;

    root_dentry = new_dentry("/", NFS_DIR);

    if (nfs_driver_read(NFS_SUPER_OFS, (uint8_t *)(&nfs_super_d), 
                        sizeof(struct nfs_super_d)) != 0) {
        return -1;
    }

    if (nfs_super_d.magic_num != NFS_MAGIC_NUM) {
        // 估算各部分大小
        super_blks = NFS_ROUND_UP(sizeof(struct nfs_super_d), NFS_BLK_SZ()) / NFS_BLK_SZ();
        map_inode_blks = 1;
        map_data_blks = 1;
        // inode 数量
        inode_num = ((NFS_DISK_SZ()/NFS_BLK_SZ()) - super_blks - 
            NFS_MAP_DATA_BLKS - NFS_MAP_INODE_BLKS)/(NFS_DATA_PER_FILE + NFS_INODE_PER_FILE);
        
        /* 布局layout */
        nfs_super_d.max_ino = inode_num;
        nfs_super_d.map_inode_blks = map_inode_blks;
        nfs_super_d.map_inode_offset = NFS_SUPER_OFS + NFS_BLKS_SZ(super_blks);
        nfs_super_d.map_data_blks = map_data_blks;
        nfs_super_d.map_data_offset = nfs_super_d.map_inode_offset + NFS_BLKS_SZ(map_inode_blks);

        nfs_super_d.inode_offset = nfs_super_d.map_data_offset + NFS_BLKS_SZ(map_data_blks);
        nfs_super_d.data_offset = nfs_super_d.inode_offset + NFS_BLKS_SZ(nfs_super_d.max_ino);
        nfs_super_d.inode_blks = inode_num;
        nfs_super_d.data_blks = inode_num*NFS_DATA_PER_FILE;
        is_init = 1;
    }
    /* 建立 in-memory 结构 */
    nfs_super.max_ino = nfs_super_d.max_ino;
    nfs_super.map_inode = (uint8_t *)malloc(NFS_BLKS_SZ(nfs_super_d.map_inode_blks));
    nfs_super.map_inode_blks = nfs_super_d.map_inode_blks;
    nfs_super.map_inode_offset = nfs_super_d.map_inode_offset;

    nfs_super.map_data = (uint8_t *)malloc(NFS_BLKS_SZ(nfs_super_d.map_data_blks));
    nfs_super.map_data_blks = nfs_super_d.map_data_blks;
    nfs_super.map_data_offset = nfs_super_d.map_data_offset;

    nfs_super.inode_blks = nfs_super_d.inode_blks;
    nfs_super.inode_offset = nfs_super_d.inode_offset;
    nfs_super.data_blks = nfs_super_d.data_blks;
    nfs_super.data_offset = nfs_super_d.data_offset;

    // 读出 inode bitmap 和 data bitmap
    if (nfs_driver_read(nfs_super_d.map_inode_offset, (uint8_t *)(nfs_super.map_inode), 
                    NFS_BLKS_SZ(nfs_super_d.map_inode_blks)) != 0) {
        return -1;
    }

    if (nfs_driver_read(nfs_super_d.map_data_offset, (uint8_t *)(nfs_super.map_data),
                    NFS_BLKS_SZ(nfs_super_d.map_data_blks)) != 0) {
        return -1;
    }

    if (is_init) {                          /* 分配根节点 */
        root_inode = nfs_alloc_inode(root_dentry);
        printf("root ino: %d", root_inode->ino);
        nfs_sync_inode(root_inode);
    }

    // 读出根节点inode
    root_inode = nfs_read_inode(root_dentry, NFS_ROOT_INO);
    printf("root ino: %d", root_inode->ino);
    root_dentry->inode    = root_inode;
    nfs_super.root_dentry = root_dentry;
    nfs_super.is_mounted  = 1;
    return 0;
}


/**
 * @brief 
 * 
 * @return int 
 */
int nfs_umount() {
    struct nfs_super_d  nfs_super_d; 

    if (!nfs_super.is_mounted) {
        return 0;
    }

    nfs_sync_inode(nfs_super.root_dentry->inode);     /* 从根节点向下刷写节点 */
                                                    
    nfs_super_d.magic_num           = NFS_MAGIC_NUM;
    nfs_super_d.max_ino             = nfs_super.max_ino;
    nfs_super_d.map_inode_blks      = nfs_super.map_inode_blks;
    nfs_super_d.map_inode_offset    = nfs_super.map_inode_offset;
    nfs_super_d.map_data_blks       = nfs_super.map_data_blks;
    nfs_super_d.map_data_offset     = nfs_super.map_data_offset;
    nfs_super_d.inode_blks          = nfs_super.inode_blks;
    nfs_super_d.inode_offset        = nfs_super.inode_offset;
    nfs_super_d.data_blks           = nfs_super.data_blks;
    nfs_super_d.data_offset         = nfs_super.data_offset;

    // 写超级块
    if (nfs_driver_write(NFS_SUPER_OFS, (uint8_t *)&nfs_super_d, 
                     sizeof(struct nfs_super_d)) != 0) {
        return -1;
    }

    // 写 inode bitmap
    if (nfs_driver_write(nfs_super_d.map_inode_offset, (uint8_t *)(nfs_super.map_inode), 
                        NFS_BLKS_SZ(nfs_super_d.map_inode_blks)) != 0) {
        return -1;
    }

    // 写 data bitmap
    if (nfs_driver_write(nfs_super_d.map_data_offset, (uint8_t *)(nfs_super.map_data),
                        NFS_BLKS_SZ(nfs_super_d.map_data_blks)) != 0){
        return -1;
    }

    // 释放bitmap
    free(nfs_super.map_inode);
    free(nfs_super.map_data);
    ddriver_close(NFS_DRIVER());

    return 0;
}

/**
 * @brief 
 * 
 * @param inode 
 * @param dir [0...]
 * @return struct sfs_dentry* 
 */
struct nfs_dentry* nfs_get_dentry(struct nfs_inode * inode, int dir) {
    struct nfs_dentry* dentry_cursor = inode->dentrys;
    int    cnt = 0;
    while (dentry_cursor)
    {
        if (dir == cnt) {
            return dentry_cursor;
        }
        cnt++;
        dentry_cursor = dentry_cursor->brother;
    }
    return NULL;
}
/**
 * @brief 
 * path: /qwe/ad  total_lvl = 2,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry 
 *      3) find qwe's inode     lvl = 2
 *      4) find ad's dentry
 *
 * path: /qwe     total_lvl = 1,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry
 * 
 * @param path 
 * @return struct nfs_inode* 
 */
struct nfs_dentry* nfs_lookup(const char * path, int* is_find, int* is_root) {
    struct nfs_dentry* dentry_cursor = nfs_super.root_dentry;
    struct nfs_dentry* dentry_ret = NULL;
    struct nfs_inode*  inode; 
    // 获取 level
    int   total_lvl = nfs_calc_lvl(path);
    int   lvl = 0;
    int is_hit;
    char* fname = NULL;
    char* path_cpy = (char*)malloc(strlen(path));
    *is_root = 0;
    strcpy(path_cpy, path);

    if (total_lvl == 0) {                           /* 根目录 */
        *is_find = 1;
        *is_root = 1;
        dentry_ret = nfs_super.root_dentry;
    }
    // strtok 函数用于分割字符串
    fname = strtok(path_cpy, "/");       
    while (fname)
    {   
        lvl++;
        if (dentry_cursor->inode == NULL) {           /* Cache机制? */
            nfs_read_inode(dentry_cursor, dentry_cursor->ino);
        }

        inode = dentry_cursor->inode;

        if (NFS_IS_REG(inode) && lvl < total_lvl) {
            NFS_DBG("[%s] not a dir\n", __func__);
            dentry_ret = inode->dentry;
            break;
        }
        if (NFS_IS_DIR(inode)) {
            dentry_cursor = inode->dentrys;
            is_hit        = 0;

            while (dentry_cursor)
            {
                if (memcmp(dentry_cursor->fname, fname, strlen(fname)) == 0) {
                    is_hit = 1;
                    break;
                }
                dentry_cursor = dentry_cursor->brother;
            }
            
            if (!is_hit) {
                *is_find = 0;
                NFS_DBG("[%s] not found %s\n", __func__, fname);
                dentry_ret = inode->dentry;
                break;
            }

            if (is_hit && lvl == total_lvl) {
                *is_find = 1;
                dentry_ret = dentry_cursor;
                break;
            }
        }
        fname = strtok(NULL, "/"); 
    }

    if (dentry_ret->inode == NULL) {
        dentry_ret->inode = nfs_read_inode(dentry_ret, dentry_ret->ino);
    }
    
    return dentry_ret;
}

// void sfs_dump_map() {
//     int byte_cursor = 0;
//     int bit_cursor = 0;

//     for (byte_cursor = 0; byte_cursor < SFS_BLKS_SZ(nfs_super.map_inode_blks); 
//          byte_cursor+=4)
//     {
//         for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
//             printf("%d ", (nfs_super.map_inode[byte_cursor] & (0x1 << bit_cursor)) >> bit_cursor);   
//         }
//         printf("\t");

//         for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
//             printf("%d ", (nfs_super.map_inode[byte_cursor + 1] & (0x1 << bit_cursor)) >> bit_cursor);   
//         }
//         printf("\t");
        
//         for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
//             printf("%d ", (nfs_super.map_inode[byte_cursor + 2] & (0x1 << bit_cursor)) >> bit_cursor);   
//         }
//         printf("\t");
        
//         for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
//             printf("%d ", (nfs_super.map_inode[byte_cursor + 3] & (0x1 << bit_cursor)) >> bit_cursor);   
//         }
//         printf("\n");
//     }
// }