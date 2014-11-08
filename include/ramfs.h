#ifndef __RAMFS_H__
#define __RAMFS_H__

#include <stdint.h>
#include <filesystem.h>

#define RAMFS_TYPE 194671278

#define MAX_INODE_BLOCK_COUNT 32
#define BLOCK_SIZE 64

typedef struct ramfs_inode_t{
    uint32_t hash;
    uint32_t device;
    uint32_t number;
    uint32_t attribute;
    char filename[64];
    uint32_t data_length;
    uint32_t block_count;
    uint32_t blocks[MAX_INODE_BLOCK_COUNT];
}ramfs_inode_t;

typedef struct ramfs_block_t {
    uint8_t data[BLOCK_SIZE];
}ramfs_block_t;

typedef struct ramfs_superblock_t{
    uint32_t device;
    uint32_t inode_count;
    uint32_t block_count;
    ramfs_inode_t** inode_list;
    ramfs_block_t** block_pool;
    struct ramfs_superblock_t* next;
}ramfs_superblock_t;

void register_ramfs();

#endif

