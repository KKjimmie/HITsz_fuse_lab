#ifndef _NFS_H_
#define _NFS_H_

#define FUSE_USE_VERSION 26
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "fcntl.h"
#include "string.h"
#include "fuse.h"
#include <stddef.h>
#include "ddriver.h"
#include "errno.h"
#include "types.h"


/******************************************************************************
* SECTION: macro debug
*******************************************************************************/

#define NFS_DBG(fmt, ...) do { printf("SFS_DBG: " fmt, ##__VA_ARGS__); } while(0) 

/******************************************************************************
* SECTION: nfs_utils.c
*******************************************************************************/

char* 					nfs_get_fname(const char*);
int 			   		nfs_calc_lvl(const char *);
struct nfs_dentry*		new_dentry(char*, NFS_FILE_TYPE);
int 					nfs_driver_read(int, uint8_t*, int);
int 					nfs_driver_write(int, uint8_t*, int);
struct nfs_inode* 		nfs_alloc_inode(struct nfs_dentry *);
int 					nfs_sync_inode(struct nfs_inode *);
int 					nfs_alloc_dentry(struct nfs_inode*, struct nfs_dentry*);
int 					nfs_alloc_data();
struct nfs_inode* 		nfs_read_inode(struct nfs_dentry *, int);
int 					nfs_mount(struct custom_options);
int 					nfs_umount();
struct nfs_dentry* 		nfs_get_dentry(struct nfs_inode * inode, int dir);
struct nfs_dentry* 		nfs_lookup(const char * path, int * is_find, int* is_root);

/******************************************************************************
* SECTION: nfs.c
*******************************************************************************/

void* 			   		nfs_init(struct fuse_conn_info *);
void  			   		nfs_destroy(void *);
int   			   		nfs_mkdir(const char *, mode_t);
int   			   		nfs_getattr(const char *, struct stat *);
int   			   		nfs_readdir(const char *, void *, fuse_fill_dir_t, off_t,
						                struct fuse_file_info *);
int   			   		nfs_mknod(const char *, mode_t, dev_t);
int   			   		nfs_write(const char *, const char *, size_t, off_t,
					                  struct fuse_file_info *);
int   			   		nfs_read(const char *, char *, size_t, off_t,
					                 struct fuse_file_info *);
int   			   		nfs_access(const char *, int);
int   			   		nfs_unlink(const char *);
int   			   		nfs_rmdir(const char *);
int   			   		nfs_rename(const char *, const char *);
int   			   		nfs_utimens(const char *, const struct timespec tv[2]);
int   			   		nfs_truncate(const char *, off_t);
			
int   			   		nfs_open(const char *, struct fuse_file_info *);
int   			   		nfs_opendir(const char *, struct fuse_file_info *);

/******************************************************************************
* SECTION: nfs_debug.c
*******************************************************************************/
// void 			   		nfs_dump_map();


#endif  /* _nfs_H_ */