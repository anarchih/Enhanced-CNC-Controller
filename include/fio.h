#ifndef __FIO_H__
#define __FIO_H__

#include <stdio.h>
#include <stdint.h>
#include <filesystem.h>

enum open_types_t {
    O_RDONLY = 0,
    O_WRONLY = 1,                                                               
    O_RDWR = 2,
    O_CREAT = 4,
    O_TRUNC = 8,
    O_APPEND = 16,
};

#define MAX_FDS 32
#define MAX_DDS 4

typedef struct dir_entity {
    uint8_t d_attr;
    char d_name[256];    
}dir_entity_t;

struct fddef_t {
    inode_t* inode;
    uint32_t flags;
    uint32_t mode;
    size_t cursor;
    void * opaque;
};

struct dddef_t {
    inode_t* inode;
    size_t cursor;
    void * opaque;
};

/* Need to be called before using any other fio functions */
__attribute__((constructor)) void fio_init();

int fio_mkdir(const char * path);
int fio_dir_is_open(int dd);
int fio_opendir(const char* path);
void fio_set_dir_opaque(int dd, void * opaque);
int fio_closedir(int dd);
off_t fio_seekdir(int fd, off_t offset);
ssize_t fio_readdir(int dd, struct dir_entity* ent);

int fio_is_open(int fd);
int fio_open(const char * path, int flags, int mode);
ssize_t fio_read(int fd, void * buf, size_t count);
ssize_t fio_write(int fd, const void * buf, size_t count);
off_t fio_seek(int fd, off_t offset, int whence);
int fio_close(int fd);
void fio_set_opaque(int fd, void * opaque);
#endif
