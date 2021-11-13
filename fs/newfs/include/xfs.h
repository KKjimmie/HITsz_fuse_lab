#ifndef _XFS_H_
#define _XFS_H_

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

#define XFS_MAGIC                  /* TODO: Define by yourself */
#define XFS_DEFAULT_PERM    0777   /* 全权限打开 */

/******************************************************************************
* SECTION: xfs.c
*******************************************************************************/
void* 			   xfs_init(struct fuse_conn_info *);
void  			   xfs_destroy(void *);
int   			   xfs_mkdir(const char *, mode_t);
int   			   xfs_getattr(const char *, struct stat *);
int   			   xfs_readdir(const char *, void *, fuse_fill_dir_t, off_t,
						                struct fuse_file_info *);
int   			   xfs_mknod(const char *, mode_t, dev_t);
int   			   xfs_write(const char *, const char *, size_t, off_t,
					                  struct fuse_file_info *);
int   			   xfs_read(const char *, char *, size_t, off_t,
					                 struct fuse_file_info *);
int   			   xfs_access(const char *, int);
int   			   xfs_unlink(const char *);
int   			   xfs_rmdir(const char *);
int   			   xfs_rename(const char *, const char *);
int   			   xfs_utimens(const char *, const struct timespec tv[2]);
int   			   xfs_truncate(const char *, off_t);
			
int   			   xfs_open(const char *, struct fuse_file_info *);
int   			   xfs_opendir(const char *, struct fuse_file_info *);

#endif  /* _xfs_H_ */