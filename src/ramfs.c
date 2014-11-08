#include <string.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <unistd.h>
#include "fio.h"
#include "filesystem.h"
#include "ramfs.h"
#include "osdebug.h"
#include "hash-djb2.h"

#include "clib.h"

typedef struct ramfs_fds_t {
    ramfs_inode_t * file_des;
    ramfs_superblock_t* sb;
    uint32_t cursor;
}ramfs_fds_t;

static uint32_t device_count = 0xAAAA; //A magic Number

//static struct ramfs_fds_t ramfs_fds[MAX_FDS];
//static struct ramfs_fds_t ramfs_dds[MAX_FDS];

ramfs_superblock_t* ramfs_sb_list = NULL;

static ramfs_superblock_t* init_superblock(void){
    ramfs_superblock_t* ret = (ramfs_superblock_t*)calloc(sizeof(ramfs_superblock_t), 1);
    ret->device = device_count++;
    ret->inode_count = 0; 
    ret->block_count = 0;
    ret->inode_list = NULL;
    ret->next = ramfs_sb_list;
    ramfs_sb_list = ret;
    return ret;
}


/*
static void deinit_superblock(ramfs_superblock_t* sb){
    if(!sb->inode_list)
        free(sb->inode_list);
    free(sb);
}
*/

static ramfs_inode_t* add_inode(const char* filename, ramfs_superblock_t* sb){
    ramfs_inode_t* ret = (ramfs_inode_t*)calloc(sizeof(ramfs_inode_t), 1);
    if(!ret)
        return NULL;
    ret->hash = hash_djb2((uint8_t*)filename, -1);
    ret->device = sb->device;
    strcpy(ret->filename, filename);
    ret->attribute = 0;
    ret->data_length = 0;
    ret->block_count = 0;
    ret->number = sb->inode_count;

    ramfs_inode_t** src = sb->inode_list;
    sb->inode_list = (ramfs_inode_t**)calloc(sizeof(ramfs_inode_t*), sb->inode_count + 1);
    memcpy(sb->inode_list, src, sizeof(ramfs_inode_t*) * sb->inode_count);
    free(src);

    sb->inode_list[sb->inode_count++] = ret;
    return ret;
}

static uint32_t add_block(ramfs_superblock_t* sb){
    ramfs_block_t* ret = (ramfs_block_t*)calloc(sizeof(ramfs_block_t), 1);

    ramfs_block_t** src = sb->block_pool;
    sb->block_pool = (ramfs_block_t**)calloc(sizeof(ramfs_block_t*), sb->block_count + 1);
    memcpy(sb->block_pool, src, sizeof(ramfs_block_t*) * sb->block_count);
    free(src);

    sb->block_pool[sb->block_count++] = ret;
    return sb->block_count - 1;
}

static ssize_t ramfs_write(struct inode_t* inode, const void* buf, size_t count, off_t offset) {
    ramfs_superblock_t* ptr = ramfs_sb_list;
    ramfs_inode_t * ramfs_node = NULL;

    while(ptr){
        if(ptr->device == inode->device){
            if(inode->number >= ptr->inode_count)
               return -1;

            ramfs_node = ptr->inode_list[inode->number];
            break;
        }
        ptr = ptr->next;
    }

    if(!ramfs_node)
        return -1;
    if(ramfs_node->attribute && 1)
        return -2;

    uint8_t* src = (uint8_t*)buf;
    uint32_t start_block_number;
    uint32_t pCount = count;     

    if(!count)
        return 0;

    start_block_number = offset >> 6;   //Every Block is 4096 Bytes
    
    if(start_block_number >= ramfs_node->block_count){
        ramfs_node->blocks[ramfs_node->block_count] = add_block(ptr);
        ramfs_node->block_count++;
    }
    memcpy(ptr->block_pool[ramfs_node->blocks[start_block_number++]]->data + (offset & (0x3F)), src, \
            count < (BLOCK_SIZE - (offset & (0x3F))) ? count : BLOCK_SIZE - (offset & (0x3F)));
    src += count < (BLOCK_SIZE - (offset & (0x3F))) ? count : BLOCK_SIZE - (offset & (0x3F));
    count -= count < (BLOCK_SIZE - (offset & (0x3F))) ? count : BLOCK_SIZE - (offset & (0x3F));

    while(count){
        if(start_block_number >= ramfs_node->block_count){
            ramfs_node->blocks[ramfs_node->block_count] = add_block(ptr);
            ramfs_node->block_count++;
        }
        memcpy(ptr->block_pool[ramfs_node->blocks[start_block_number++]]->data, src, (count > BLOCK_SIZE ? BLOCK_SIZE: count));
        src += (count > BLOCK_SIZE ? BLOCK_SIZE: count);
        count -= (count > BLOCK_SIZE ? BLOCK_SIZE: count);
    }

    offset += pCount;
    if(offset > ramfs_node->data_length)
        ramfs_node->data_length += offset - ramfs_node->data_length;

    return pCount;
}

static ssize_t ramfs_read(struct inode_t* inode, void* buf, size_t count, off_t offset) {
    ramfs_superblock_t* ptr = ramfs_sb_list; 
    ramfs_inode_t * ramfs_node = NULL;

    while(ptr){
        if(ptr->device == inode->device){
            if(inode->number >= ptr->inode_count)
               return -1;

            ramfs_node = ptr->inode_list[inode->number];
            break;
        }
        ptr = ptr->next;
    }

    if(!ramfs_node)
        return -1;
    if(ramfs_node->attribute && 1)
        return -2;

    uint8_t* des = (uint8_t*)buf;
    uint32_t size = ramfs_node->data_length;
    uint32_t start_block_number;
    
    if(!count)
        return 0;

    if ((offset + count) > size)
        count = size - offset;

    uint32_t pCount = count;

    start_block_number = offset >> 6;   //Every Block is 4096 Bytes
    
    memcpy(des, ptr->block_pool[ramfs_node->blocks[start_block_number++]]->data + (offset & (0x3F)), \
            count < (BLOCK_SIZE - (offset & (0x3F))) ? count : BLOCK_SIZE - (offset & (0x3F)));
    des += count < (BLOCK_SIZE - (offset & (0x3F))) ? count : BLOCK_SIZE - (offset & (0x3F));
    count -= count < (BLOCK_SIZE - (offset & (0x3F))) ? count : BLOCK_SIZE - (offset & (0x3F));

    while(count){
        memcpy(des, ptr->block_pool[ramfs_node->blocks[start_block_number++]]->data, (count > BLOCK_SIZE ? BLOCK_SIZE: count));
        des += (count > BLOCK_SIZE ? BLOCK_SIZE: count);
        count -= (count > BLOCK_SIZE ? BLOCK_SIZE: count);
    }

    return pCount;
}

static ssize_t ramfs_readdir(struct inode_t* inode, dir_entity_t* ent, off_t offset) {
    ramfs_superblock_t* ptr = ramfs_sb_list; 
    ramfs_inode_t * ramfs_node = NULL;

    while(ptr){
        if(ptr->device == inode->device){
            if(inode->number >= ptr->inode_count)
               return -1;

            ramfs_node = ptr->inode_list[inode->number];
            break;
        }
        ptr = ptr->next;
    }

    if(!ramfs_node)
        return -1;

    if(offset >= ramfs_node->block_count)
        return -2;

    ramfs_inode_t * subfile_node = NULL;
    
    for(uint32_t i = 0; i < ptr->inode_count; i++){
        if(ptr->inode_list[i]->hash == ramfs_node->blocks[offset]){
            subfile_node = ptr->inode_list[i];
            break;
        }
    }

    strcpy(ent->d_name, subfile_node->filename);
    ent->d_attr = subfile_node->attribute;

    return 0;
}


off_t ramfs_seek(struct inode_t* node, off_t offset) {
    ramfs_superblock_t* ptr = ramfs_sb_list; 
    ramfs_inode_t * ramfs_node = NULL;

    while(ptr){
        if(ptr->device == node->device){
            if(node->number >= ptr->inode_count)
               return -1;

            ramfs_node = ptr->inode_list[node->number];
            break;
        }
        ptr = ptr->next;
    }

    if(!ramfs_node)
        return -1;

    uint32_t size;
    if(ramfs_node->attribute && 1)
        size = ramfs_node->block_count;
    else
        size = ramfs_node->data_length;
    
    if(offset > size)
        offset = size;
    if(offset < 0)
        offset = 0;

    return offset;
}

/*

static off_t ramfs_seekdir(void * opaque, off_t offset) {
    struct ramfs_fds_t * dir = (struct ramfs_fds_t *) opaque;
    uint32_t file_count = *(dir->file_des->blocks);

    if(offset >= file_count || offset < 0)
        return -2;

    dir->cursor = offset;

    return offset;
}

ramfs_inode_t * ramfs_get_file_by_hash(const ramfs_superblock_t* ramfs, uint32_t h) {
    for (uint32_t i = 0; i < ramfs->inode_count; i++) {
        if (ramfs->inode_list[i]->hash == h) {
            return ramfs->inode_list[i];
        }
    }
    return NULL;
}

static int ramfs_open(void * opaque, const char * path, int flags, int mode) {
    uint32_t h = hash_djb2((const uint8_t *) path, -1);
    ramfs_superblock_t* sb = (ramfs_superblock_t*) opaque;
    ramfs_inode_t * file;
    int r = -1;

    file = ramfs_get_file_by_hash(sb, h);
    
    if(!file){
        file = add_inode(h, path, sb); 
    }

    if (file) {
        r = fio_open(ramfs_read, ramfs_write, ramfs_seek, NULL, NULL);
        if (r > 0) {
            ramfs_fds[r].file_des = file;
            ramfs_fds[r].sb = sb;
            ramfs_fds[r].cursor = 0;
            fio_set_opaque(r, ramfs_fds + r);
        }
    }
    return r;
}

static int ramfs_opendir(void * opaque, char * path) {
    uint32_t h = hash_djb2((const uint8_t *) path, -1);
    ramfs_superblock_t* sb = (ramfs_superblock_t*) opaque;
    ramfs_inode_t * dir;
    int r = -1;

    dir = ramfs_get_file_by_hash(sb, h);
    if(!(dir->attribute && 1)) //This is not a directory
        return -1;

    if (dir) {
        r = fio_opendir(ramfs_readdir, ramfs_seekdir, NULL, NULL);
        if (r >= 0) {
            ramfs_dds[r].file_des = dir;
            ramfs_dds[r].sb = sb;
            ramfs_dds[r].cursor = 0;
            fio_set_dir_opaque(r, ramfs_dds + r);
        }
    }
    return r;
}
*/

int ramfs_i_create(struct inode_t* inode, const char* fn){
    ramfs_inode_t* p_inode,* c_inode; 
    ramfs_superblock_t* ptr = ramfs_sb_list;
    while(ptr){
        if(ptr->device == inode->device){
            if(inode->number >= ptr->inode_count)
               return -1;

            p_inode = ptr->inode_list[inode->number];
            if(!(p_inode->attribute & 1))
                return -2;

            c_inode = add_inode(fn, ptr);
            p_inode->blocks[p_inode->block_count++] = c_inode->hash;
            return 0;
        }
        ptr = ptr->next;
    }
    return -4;
}

int ramfs_i_mkdir(struct inode_t* inode, const char* fn){
    ramfs_inode_t* p_inode,* c_inode; 
    ramfs_superblock_t* ptr = ramfs_sb_list;
    while(ptr){
        if(ptr->device == inode->device){
            if(inode->number >= ptr->inode_count)
               return -1;

            p_inode = ptr->inode_list[inode->number];
            if(!(p_inode->attribute & 1))
                return -2;

            c_inode = add_inode(fn, ptr);
            c_inode->attribute |= 1; //Set as Floder
            p_inode->blocks[p_inode->block_count++] = c_inode->hash;
            return 0;
        }
        ptr = ptr->next;
    }
    return -4;
}
int ramfs_i_lookup(struct inode_t* inode, const char* path){
    const char* slash = strchr(path, '/');
    uint32_t hash = hash_djb2((uint8_t*)path, (slash == NULL ? -1 : (slash - path)));

    ramfs_inode_t* ramfs_inode; 
    ramfs_superblock_t* ptr = ramfs_sb_list;
    while(ptr){
        if(ptr->device == inode->device){
            if(inode->number >= ptr->inode_count)
               return -1;

            ramfs_inode = ptr->inode_list[inode->number];
            if(!(ramfs_inode->attribute & 1))
                return -2;
            for(uint32_t i = 0; i < ramfs_inode->block_count; i++){
                if(!(ramfs_inode->attribute && 1))
                    return -1;
                if(ramfs_inode->blocks[i] == hash){
                    for(uint32_t j = 0; j < ptr->inode_count; j++){
                        if(ptr->inode_list[j]->hash == hash){
                            return j; 
                        }
                    }
               }
            }
            return -3;
        }
        ptr = ptr->next;
    }
    return -4;
}

int ramfs_read_inode(inode_t* inode){
    ramfs_superblock_t* ptr = ramfs_sb_list;
    while(ptr){
        if(ptr->device == inode->device){
            if(inode->number >= ptr->inode_count)
               return -1;
            //inode->size = ptr->inode_list[inode->number]->data_length;
            inode->mode = ptr->inode_list[inode->number]->attribute;
            inode->block_size = BLOCK_SIZE;
            inode->inode_ops.i_lookup = ramfs_i_lookup;
            inode->inode_ops.i_create = ramfs_i_create;
            inode->inode_ops.i_mkdir = ramfs_i_mkdir;
            inode->file_ops.lseek = ramfs_seek;
            inode->file_ops.read = ramfs_read;
            inode->file_ops.write = ramfs_write;
            inode->file_ops.readdir = ramfs_readdir;

            return 0;
        }
        ptr = ptr->next;
    }

    return -2;
}

int ramfs_read_superblock(void* opaque, struct superblock_t* sb){
    ramfs_superblock_t* ramfs_sb = init_superblock();
    ramfs_inode_t* ramfs_in;
    if(ramfs_sb){
        ramfs_in = add_inode("", ramfs_sb);
        ramfs_in->attribute = 1;
        if(ramfs_in){
            sb->device = ramfs_sb->device;
            sb->mounted = ramfs_in->number;
            sb->block_size = BLOCK_SIZE;
            sb->type_hash = RAMFS_TYPE;
            sb->superblock_ops.s_read_inode = ramfs_read_inode;
            return 0;
        }
    }

    return -1;
}

static fs_type_t ramfs_r = {
    .type_name_hash = RAMFS_TYPE,
    .rsbcb = ramfs_read_superblock,
    .require_dev = 0,
    .next = NULL,
};

void register_ramfs(const char * mountpoint) {
//    DBGOUT("Registering ramfs `%s' @ %p\r\n", mountpoint, ramfs);
    register_fs(&ramfs_r);
}
