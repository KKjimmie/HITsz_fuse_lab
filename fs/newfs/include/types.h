#ifndef _TYPES_H_
#define _TYPES_H_


/******************************************************************************
* SECTION: Type def
*******************************************************************************/
typedef unsigned char   uint8_t;
typedef unsigned int    uint32_t;

typedef enum nfs_file_type {
    NFS_FILE, // 文件类型
    NFS_DIR, // 目录类型
    NFS_SYM_LINK // 符号链接
} NFS_FILE_TYPE;

/******************************************************************************
* SECTION: Macro
*******************************************************************************/
#define UINT32_BITS             32
#define UINT8_BITS              8
#define MAX_NAME_LEN            128     
#define NFS_MAP_INODE_BLKS      1
#define NFS_MAP_DATA_BLKS       1
#define NFS_INODE_PER_FILE      1
#define NFS_DATA_PER_FILE       6
#define NFS_DEFAULT_PERM        0777

#define NFS_MAGIC_NUM           0320
#define NFS_SUPER_OFS           0 // super block offset
#define NFS_ROOT_INO            0


#define NFS_ERROR_NONE          0
#define NFS_ERROR_ACCESS        EACCES
#define NFS_ERROR_SEEK          ESPIPE     
#define NFS_ERROR_ISDIR         EISDIR
#define NFS_ERROR_NOSPACE       ENOSPC
#define NFS_ERROR_EXISTS        EEXIST
#define NFS_ERROR_NOTFOUND      ENOENT
#define NFS_ERROR_UNSUPPORTED   ENXIO
#define NFS_ERROR_IO            EIO     /* Error Input/Output */
#define NFS_ERROR_INVAL         EINVAL  /* Invalid Args */


/******************************************************************************
* SECTION: Macro Function
*******************************************************************************/

#define NFS_IO_SZ()                     (nfs_super.sz_io)
#define NFS_DISK_SZ()                   (nfs_super.sz_disk)
#define NFS_DRIVER()                    (nfs_super.driver_fd)
#define NFS_BLK_SZ()                    (nfs_super.sz_block)
#define NFS_DRIVER()                    (nfs_super.driver_fd)

// 取整
#define NFS_ROUND_DOWN(value, round)    (value % round == 0 ? value : (value / round) * round)
#define NFS_ROUND_UP(value, round)      (value % round == 0 ? value : (value / round + 1) * round)

#define NFS_BLKS_SZ(blks)               (blks * NFS_BLK_SZ())
#define NFS_ASSIGN_FNAME(pnfs_dentry, _fname)\
                                        memcpy(pnfs_dentry->fname, _fname, strlen(_fname))

#define NFS_INO_OFS(ino)                (nfs_super.inode_offset + NFS_BLKS_SZ(ino))
#define NFS_DATA_OFS_I(ino)             (nfs_super.data_offset + NFS_BLKS_SZ(ino*NFS_DATA_PER_FILE))
#define NFS_DATA_OFS(data_cursor)       (nfs_super.data_offset + NFS_BLKS_SZ(data_cursor))

#define NFS_IS_DIR(pinode)              (pinode->dentry->ftype == NFS_DIR)
#define NFS_IS_REG(pinode)              (pinode->dentry->ftype == NFS_FILE)
// #define NFS_IS_SYM_LINK(pinode)         (pinode->dentry->ftype == NFS_SYM_LINK)

/******************************************************************************
* SECTION: FS Specific Structure - In memory structure
*******************************************************************************/
struct nfs_super;
struct nfs_inode;
struct nfs_dentry;

struct custom_options {
    char*        device;  // 驱动的路径
};

struct nfs_super {
    uint32_t           magic;
    int                driver_fd; // ddrive fd
    int                sz_disk; // 磁盘大小，B 为单位
    int                sz_io; // 设备单次 IO 大小，B为单位（512B）
    int                sz_block; //  一块大小，B 为单位（1024B）
    int                max_ino; // 最多支持文件数
    uint8_t*           map_inode;
    int                map_inode_blks; // inode map 占用块数，以 1KB 为一块
    int                map_inode_offset; // inode map 在磁盘上的偏移，以 1KB 为单位
    uint8_t*           map_data;
    int                map_data_blks; // data map 占用块数 
    int                map_data_offset; // data map 在磁盘上的偏移
    int                inode_blks;                  // inode 占用块数
    int                inode_offset;                // inode 偏移
    int                data_blks;                   // data 占用块数
    int                data_offset;                 // data 偏移

    int      is_mounted; // 是否已挂载
    struct nfs_dentry* root_dentry;
    /* TODO: Define yourself */
};

struct nfs_inode {
    uint32_t                ino;
    int                     size;                 // 大小，以B为单位
    int                     link;                 // 链接数
    NFS_FILE_TYPE           ftype;                // 文件类型
    int                     dir_cnt;              // 如果是目录类型，下面有几个目录项
    struct nfs_dentry*      dentry;               // 指向该inode的dentry
    struct nfs_dentry*      dentrys;              // 所有目录项  
    uint8_t*                data;
    int                     block_pointer[6];     // 数据块指针（可固定分配）
    int                     dirty[6];             // 数据块脏位
    int                     block_allocated;      // 已分配数据块
};

struct nfs_dentry {
    char                    fname[MAX_NAME_LEN];   // 文件名
    NFS_FILE_TYPE           ftype;                 // 文件类型
    uint32_t                ino;                   // 指向的 ino 号
    int                     valid;                 // 该目录项是否有效
    struct nfs_inode*       inode;                 // 指向inode 
    struct nfs_dentry*      parent;
    struct nfs_dentry*      brother;
};

/******************************************************************************
* SECTION: FS Specific Structure - Disk structure
*******************************************************************************/

struct nfs_super_d
{
    uint32_t           magic_num;                   // 幻数

    int                max_ino;                     // 最多支持的文件数

    int                map_inode_blks;              // inode位图占用的块数
    int                map_inode_offset;            // inode位图在磁盘上的偏移

    int                map_data_blks;               // data位图占用的块数
    int                map_data_offset;             // data位图在磁盘上的偏移
    
    int                inode_blks;                  // inode 占用块数
    int                inode_offset;                // inode 偏移
    int                data_blks;                   // data 占用块数
    int                data_offset;                 // data 偏移
};

struct nfs_inode_d
{
    int                ino;                // 在inode位图中的下标
    int                size;               // 文件已占用空间
    int                link;               // 链接数
    NFS_FILE_TYPE      ftype;              // 文件类型（目录类型、普通文件类型）
    int                dir_cnt;            // 如果是目录类型文件，下面有几个目录项
    int                block_pointer[6];   // 数据块指针（可固定分配）
    int                block_allocated;    // 已分配数据块个数
};  

struct nfs_dentry_d
{
    char               fname[MAX_NAME_LEN];          // 指向的ino文件名
    NFS_FILE_TYPE      ftype;                        // 指向的ino文件类型
    int                ino;                          // 指向的ino号
    int                valid;                        // 该目录项是否有效
};  

#endif /* _TYPES_H_ */