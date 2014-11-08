#ifndef __FILESYSTEM_H__
#define __FILESYSTEM_H__

#include <stdint.h>
#include <hash-djb2.h>
#include <FreeRTOS.h>
#include <semphr.h>

#define MAX_FS 16
#define OPENFAIL (-1)

struct dir_entity;

typedef struct inode_t{
    uint32_t device;
    uint32_t number;
    uint32_t mode;
    uint32_t block_size;
    struct inode_operations{
        int (*i_create)(struct inode_t* node, const char* path);
        int (*i_lookup)(struct inode_t* node, const char* path);
        int (*i_mkdir)(struct inode_t* node, const char* path);
    }inode_ops;
    uint32_t count;
    xSemaphoreHandle lock;
    struct file_operations{
        off_t (*lseek)(struct inode_t* node, off_t offset);
        ssize_t (*read)(struct inode_t* node, void* buf, size_t count, off_t offset);
        ssize_t (*write)(struct inode_t* node, const void* buf, size_t count, off_t offset);
        ssize_t (*readdir)(struct inode_t* node, struct dir_entity* filldir, off_t offset);
    }file_ops;
    void* opaque;
}inode_t;

typedef struct superblock_t{
    uint32_t device;
    uint32_t mounted;
    inode_t* covered;
    uint32_t block_size;
    uint32_t type_hash;
    struct superblock_operations{
        int (*s_read_inode)(struct inode_t* inode);
        int (*s_write_inode)(struct inode_t* inode);
        int (*s_umount)(void);
    }superblock_ops;
    void* opaque;
}superblock_t;

typedef struct fs_type_t {
    uint32_t type_name_hash;
    int (*rsbcb)(void* opaque, struct superblock_t* sb);
    uint32_t require_dev; 
    struct fs_type_t* next;
}fs_type_t;

/*
typedef int (*fs_open_t)(void * opaque, const char * fname, int flags, int mode);
typedef int (*fs_opendir_t)(void * opaque, char* path);
*/
/* Need to be called before using any other fs functions */
__attribute__((constructor)) void fs_init();

int register_fs(fs_type_t* type);
int fs_mount(inode_t* mountpoint, uint32_t type, void* opaque);
int get_inode_by_path(const char* path, inode_t** inode);
inode_t* fs_get_inode(uint32_t device, uint32_t number);
void fs_free_inode(inode_t* inode);

/*
int fs_open(const char * path, int flags, int mode);
int fs_opendir(char * path);
int fs_list(const char * path, char*** ret_path);
*/

#endif
