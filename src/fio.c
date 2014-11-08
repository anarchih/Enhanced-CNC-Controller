#include <string.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <unistd.h>
#include "devfs.h"
#include "fio.h"
#include "filesystem.h"
#include "osdebug.h"
#include "hash-djb2.h"

static struct fddef_t fio_fds[MAX_FDS];
static struct dddef_t fio_dds[MAX_DDS];

static xSemaphoreHandle fio_sem = NULL;

struct fddef_t * fio_getfd(int fd) {
    if ((fd < 0) || (fd >= MAX_FDS))
        return NULL;
    return fio_fds + fd;
}

static int fio_is_open_int(int fd) {
    if ((fd < 0) || (fd >= MAX_FDS))
        return 0;
    int r = !(fio_fds[fd].inode == NULL);
    return r;
}

static int fio_is_dir_open_int(int dd) {
    if ((dd < 0) || (dd >= MAX_DDS))
        return 0;
    int r = !((fio_dds[dd].inode == NULL));
    return r;
}

static int fio_findfd() {
    int i;
    
    for (i = 0; i < MAX_FDS; i++) {
        if (!fio_is_open_int(i))
            return i;
    }
    
    return -1;
}

static int fio_finddd() {
    int i;
    
    for (i = 0; i < MAX_DDS; i++) {
        if (!fio_is_dir_open_int(i))
            return i;
    }
    
    return -1;
}

int fio_is_open(int fd) {
    int r = 0;
    xSemaphoreTake(fio_sem, portMAX_DELAY);
    r = fio_is_open_int(fd);
    xSemaphoreGive(fio_sem);
    return r;
}

int fio_open(const char * path, int flags, int mode) {
    int fd, ret, target_node;
    inode_t* p_inode,* f_inode;
    const char* fn = path + strlen(path) - 1;
    char buf[64], fn_buf[128];
    
    if(strcmp(path, "/") == 0)
        return -1;

    ret = 0;
    while(*fn == '/')fn--, ret++;
    while(*fn != '/')fn--;
    fn++;
    strncpy(fn_buf, fn, strlen(fn) - ret);
    fn_buf[strlen(fn) - ret] = '\0';

    strncpy(buf, path, fn - path);
    buf[fn - path] = '\0';

//    DBGOUT("fio_open(%p, %p, %p, %p, %p)\r\n", fdread, fdwrite, fdseek, fdclose, opaque);
    ret = get_inode_by_path(buf, &p_inode);
    if(!ret){
        target_node = p_inode->inode_ops.i_lookup(p_inode, fn_buf);

        if(target_node){
            if(p_inode->inode_ops.i_create){
                xSemaphoreTake(p_inode->lock, portMAX_DELAY);
                if(p_inode->inode_ops.i_create(p_inode, fn_buf)){
                    xSemaphoreGive(p_inode->lock);
                    fs_free_inode(p_inode);
                    return -3;       
                }else{
                    xSemaphoreGive(p_inode->lock);
                    target_node = p_inode->inode_ops.i_lookup(p_inode, fn_buf);
                }
            }else{
                fs_free_inode(p_inode);
                return -2;
            }
        }

        f_inode = fs_get_inode(p_inode->device, target_node);
        
        if(f_inode->mode && 1){
            fs_free_inode(f_inode);
            fs_free_inode(p_inode);
            return -4;
        }

        xSemaphoreTake(fio_sem, portMAX_DELAY);
        fd = fio_findfd();
            
        if (fd >= 0) {
            fio_fds[fd].inode = f_inode;
            fio_fds[fd].flags = flags;
            fio_fds[fd].mode = mode;
            fio_fds[fd].cursor = 0;
            fio_fds[fd].opaque = NULL;
        }
        xSemaphoreGive(fio_sem);

        fs_free_inode(p_inode);

        return fd;
    }else{
        return -1;
    }
}

int fio_mkdir(const char * path) {
    int ret, target_node;
    inode_t* p_inode;
    const char* fn = path + strlen(path) - 1;
    char buf[64], fn_buf[128];

    if(strcmp(path, "/") == 0)
        return -1;

    ret = 0;
    while(*fn == '/')fn--, ret++;
    while(*fn != '/')fn--;
    fn++;
    strncpy(fn_buf, fn, strlen(fn) - ret);
    fn_buf[strlen(fn) - ret] = '\0';

    strncpy(buf, path, fn - path);
    buf[fn - path] = '\0';

//    DBGOUT("fio_open(%p, %p, %p, %p, %p)\r\n", fdread, fdwrite, fdseek, fdclose, opaque);
    ret = get_inode_by_path(buf, &p_inode);
    if(!ret){
        target_node = p_inode->inode_ops.i_lookup(p_inode, fn_buf);

        if(!target_node){
            return -1;
        }else{
            if(p_inode->inode_ops.i_mkdir){
                xSemaphoreTake(p_inode->lock, portMAX_DELAY);
                if(p_inode->inode_ops.i_mkdir(p_inode, fn_buf)){
                    xSemaphoreGive(p_inode->lock);
                    fs_free_inode(p_inode);
                    return -3;       
                }else{
                    xSemaphoreGive(p_inode->lock);
                    fs_free_inode(p_inode);
                    return 0;       
                }
            }else{
                fs_free_inode(p_inode);
                return -2;
            }
        }
    }else{
        return -1;
    }
}

int fio_opendir(const char* path) {
    int dd, ret, target_node;
    inode_t* p_inode,* f_inode;
    const char* fn = path + strlen(path) - 1;
    char buf[64], fn_buf[128];

    if(strcmp(path, "/") == 0){
        ret = get_inode_by_path(path, &p_inode);
        xSemaphoreTake(fio_sem, portMAX_DELAY);
        dd = fio_finddd();
            
        if (dd >= 0) {
            fio_dds[dd].inode = p_inode;
            fio_dds[dd].cursor = 0;
            fio_dds[dd].opaque = NULL;
        }
        xSemaphoreGive(fio_sem);
        return dd;
    }else{
        ret = 0;
        while(*fn == '/')fn--, ret++;
        while(*fn != '/')fn--;
        fn++;
        strncpy(fn_buf, fn, strlen(fn) - ret);
        fn_buf[strlen(fn) - ret] = '\0';

        strncpy(buf, path, fn - path);
        buf[fn - path] = '\0';
        
        ret = get_inode_by_path(buf, &p_inode);
        if(!ret){
            target_node = p_inode->inode_ops.i_lookup(p_inode, fn_buf);

            if(!target_node){
                return -1;
            }

            f_inode = fs_get_inode(p_inode->device, target_node);
            
            if(!(f_inode->mode && 1)){
                fs_free_inode(f_inode);
                fs_free_inode(p_inode);
                return -4;
            }

            xSemaphoreTake(fio_sem, portMAX_DELAY);
            dd = fio_finddd();
                
            if (dd >= 0) {
                fio_dds[dd].inode = f_inode;
                fio_dds[dd].cursor = 0;
                fio_dds[dd].opaque = NULL;
            }
            xSemaphoreGive(fio_sem);

            fs_free_inode(p_inode);

            return dd;
        }else{
            return -1;
        }
    }
}


ssize_t fio_read(int fd, void * buf, size_t count) {
    ssize_t r = 0;
//    DBGOUT("fio_read(%i, %p, %i)\r\n", fd, buf, count);
    if (fio_is_open_int(fd)) {
        if (fio_fds[fd].inode->file_ops.read) {
            xSemaphoreTake(fio_fds[fd].inode->lock, portMAX_DELAY);
            r = fio_fds[fd].inode->file_ops.read(fio_fds[fd].inode, buf, count, fio_fds[fd].cursor);
            fio_fds[fd].cursor += r;
            xSemaphoreGive(fio_fds[fd].inode->lock);
        }
    } else {
        r = -2;
    }
    return r;
}


ssize_t fio_readdir(int dd, struct dir_entity* ent) {
    ssize_t r = 0;
//    DBGOUT("fio_read(%i, %p, %i)\r\n", fd, buf, count);
    if (fio_is_dir_open_int(dd)) {
        if (fio_dds[dd].inode->file_ops.readdir) {
            r = fio_dds[dd].inode->file_ops.readdir(fio_dds[dd].inode, ent, fio_dds[dd].cursor++);
        } else {
            r = -3;
        }
    } else {
        r = -2;
    }
    return r;
}


ssize_t fio_write(int fd, const void * buf, size_t count) {
    ssize_t r = 0;
//    DBGOUT("fio_write(%i, %p, %i)\r\n", fd, buf, count);
    if (fio_is_open_int(fd)) {
        if (fio_fds[fd].inode->file_ops.write) {
            xSemaphoreTake(fio_fds[fd].inode->lock, portMAX_DELAY);
            r = fio_fds[fd].inode->file_ops.write(fio_fds[fd].inode, buf, count, fio_fds[fd].cursor);
            fio_fds[fd].cursor += r;
            xSemaphoreGive(fio_fds[fd].inode->lock);
        } else {
            r = -3;
        }
    } else {
        r = -2;
    }
    return r;
}

off_t fio_seek(int fd, off_t offset, int whence) {
//    DBGOUT("fio_seek(%i, %i, %i)\r\n", fd, offset, whence);
    if (fio_is_open_int(fd)) {
        
        uint32_t origin;
        
        switch (whence) {
        case SEEK_SET:
            origin = 0;
            break;
        case SEEK_CUR:
            origin = fio_fds[fd].cursor;
            break;
        case SEEK_END:
            origin = 0xFFFFFFFF;
            break;
        default:
            return -1;
        }

        offset = origin + offset;
        
        if(!fio_fds[fd].inode->file_ops.lseek)
            return -1;

        offset = fio_fds[fd].inode->file_ops.lseek(fio_fds[fd].inode, offset);

        xSemaphoreTake(fio_sem, portMAX_DELAY);
        fio_fds[fd].cursor = offset;
        xSemaphoreGive(fio_sem);
        return offset;
    } else {
        return -2;
    }
    return -3;
}


off_t fio_seekdir(int dd, off_t offset) {
    if (fio_is_dir_open_int(dd)) {
        if(!fio_dds[dd].inode->file_ops.lseek)
            return -1;

        offset = fio_dds[dd].inode->file_ops.lseek(fio_dds[dd].inode, offset);

        xSemaphoreTake(fio_sem, portMAX_DELAY);
        fio_dds[dd].cursor = offset;
        xSemaphoreGive(fio_sem);

        return 0;
    } else {
        return -2;
    }
    return -3;
}


int fio_close(int fd) {
    int r = 0;
//    DBGOUT("fio_close(%i)\r\n", fd);
    if (fio_is_open_int(fd)) {
//        if (fio_fds[fd].fdclose)
  //          r = fio_fds[fd].fdclose(fio_fds[fd].opaque);
        xSemaphoreTake(fio_sem, portMAX_DELAY);
        fs_free_inode(fio_fds[fd].inode);
        memset(fio_fds + fd, 0, sizeof(struct fddef_t));
        xSemaphoreGive(fio_sem);
    } else {
        r = -2;
    }
    return r;
}

int fio_closedir(int dd) {
//    DBGOUT("fio_close(%i)\r\n", fd);
    if (fio_is_dir_open_int(dd)) {
        fs_free_inode(fio_dds[dd].inode);
        xSemaphoreTake(fio_sem, portMAX_DELAY);
        memset(fio_dds + dd, 0, sizeof(struct dddef_t));
        xSemaphoreGive(fio_sem);
        return 0;
    } else {
        return -1;
    }
}

void fio_set_opaque(int fd, void * opaque) {
    if (fio_is_open_int(fd))
        fio_fds[fd].opaque = opaque;
}

void fio_set_dir_opaque(int dd, void * opaque) {
    if (fio_is_dir_open_int(dd))
        fio_dds[dd].opaque = opaque;
}


/*
static int devfs_open(void * opaque, const char * path, int flags, int mode) {
    uint32_t h = hash_djb2((const uint8_t *) path, -1);
//    DBGOUT("devfs_open(%p, \"%s\", %i, %i)\r\n", opaque, path, flags, mode);
    switch (h) {
    case stdin_hash:
        if (flags & (O_WRONLY | O_RDWR))
            return -1;
        return fio_open(stdin_read, NULL, NULL, NULL, NULL);
        break;
    case stdout_hash:
        if (flags & O_RDONLY)
            return -1;
        return fio_open(NULL, stdout_write, NULL, NULL, NULL);
        break;
    case stderr_hash:
        if (flags & O_RDONLY)
            return -1;
        return fio_open(NULL, stdout_write, NULL, NULL, NULL);
        break;
    }
    return -1;
}
*/

__attribute__((constructor)) void fio_init() {
    memset(fio_fds, 0, sizeof(fio_fds));
    fio_fds[0].inode = get_stdin_node();
    fio_fds[0].inode->lock = xSemaphoreCreateMutex();
    fio_fds[1].inode = get_stdout_node();
    fio_fds[1].inode->lock = xSemaphoreCreateMutex();
    fio_fds[2].inode = get_stderr_node();
    fio_fds[2].inode->lock = xSemaphoreCreateMutex();
    fio_sem = xSemaphoreCreateMutex();
}

