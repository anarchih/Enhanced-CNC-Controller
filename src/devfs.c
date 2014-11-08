#include <string.h>
#include <FreeRTOS.h>
#include <unistd.h>
#include "fio.h"
#include "devfs.h"
#include "filesystem.h"
#include "osdebug.h"
#include "hash-djb2.h"

superblock_t dev_superblock = {
    .device = 0xDEADBEEF,
    .mounted = 0,
    .covered = NULL,
    .block_size = 0,
    .type_hash = 164136743,
    .superblock_ops = {
        devfs_read_inode,
        NULL,
        NULL
    },
    NULL
};

inode_t devfs_stdin_node = {
    .device = 0xDEADBEEF,
    .number = 1,
    .mode = 0,
    .block_size = 0,
    .inode_ops = {
        NULL,
        NULL,
        NULL
    },
    0,
    NULL,
    .file_ops = {
        NULL,
        stdin_read,
        NULL,
        NULL
    },
    NULL
};

inode_t devfs_stdout_node = {
    .device = 0xDEADBEEF,
    .number = 2,
    .mode = 0,
    .block_size = 0,
    .inode_ops = {
        NULL,
        NULL,
        NULL,
    },
    0,
    NULL,
    .file_ops = {
        NULL,
        NULL,
        stdout_write,
        NULL
    },
    NULL
};

inode_t devfs_stderr_node = {
    .device = 0xDEADBEEF,
    .number = 3,
    .mode = 0,
    .block_size = 0,
    .inode_ops = {
        NULL,
        NULL,
        NULL
    },
    0,
    NULL,
    .file_ops = {
        NULL,
        NULL,
        stdout_write,
        NULL
    },
    NULL
};

inode_t devfs_root_node = {
    .device = 0xDEADBEEF,
    .number = 0,
    .mode = 1,
    .block_size = 0,
    .inode_ops = {
        NULL,
        devfs_root_lookup,
        NULL
    },
    0,
    NULL,
    .file_ops = {
        NULL,
        NULL,
        NULL,
        NULL
    },
    NULL
};

fs_type_t devfs_r = {
    .type_name_hash = 164136743,
    .rsbcb = devfs_read_superblock,
    .require_dev = 0,
    .next = NULL,
};

/* recv_byte is define in main.c */
char recv_byte();
void send_byte(char);

enum KeyName{ESC=27, BACKSPACE=127};

/* Imple */
ssize_t stdin_read(struct inode_t* node, void* buf, size_t count, off_t offset) {
    int i=0, endofline=0, last_chr_is_esc;
    char *ptrbuf=buf;
    char ch;
    while(i < count&&endofline!=1){
	ptrbuf[i]=recv_byte();
	switch(ptrbuf[i]){
		case '\r':
		case '\n':
			ptrbuf[i]='\0';
			endofline=1;
			break;
		case '[':
			if(last_chr_is_esc){
				last_chr_is_esc=0;
				ch=recv_byte();
				if(ch>=1&&ch<=6){
					ch=recv_byte();
				}
				continue;
			}
		case ESC:
			last_chr_is_esc=1;
			continue;
		case BACKSPACE:
			last_chr_is_esc=0;
			if(i>0){
				send_byte('\b');
				send_byte(' ');
				send_byte('\b');
				--i;
			}
			continue;
		default:
			last_chr_is_esc=0;
	}
	send_byte(ptrbuf[i]);
	++i;
    }
    return i;
}

ssize_t stdout_write(struct inode_t* node, const void* buf, size_t count, off_t offset) {
    int i;
    const char * data = (const char *) buf;
    
    for (i = 0; i < count; i++)
        send_byte(data[i]);
    
    return count;
}

int devfs_root_lookup(struct inode_t* node, const char* path){
    const char* slash = strchr(path, '/');
    uint32_t hash = hash_djb2((uint8_t*)path, (uint32_t)(slash - path));
    switch(hash){
        case STDIN_HASH:
            return devfs_stdin_node.number;
        case STDOUT_HASH:
            return devfs_stdout_node.number;
        case STDERR_HASH:
            return devfs_stderr_node.number;
        default:
            return -1;
    }
}

int devfs_read_inode(inode_t* inode){
    switch(inode->number){
        case 0:
            memcpy(inode, &devfs_root_node, sizeof(inode_t));
            break;
        case 1:
            memcpy(inode, &devfs_stdin_node, sizeof(inode_t));
            break;
        case 2:
            memcpy(inode, &devfs_stdout_node, sizeof(inode_t));
            break;
        case 3:
            memcpy(inode, &devfs_stderr_node, sizeof(inode_t));
            break;
        default:
           return -1;
    }
    return 0;
}

int devfs_read_superblock(void* opaque, struct superblock_t* sb){
    sb = &dev_superblock;
    return 0;
}

void register_devfs() {
    DBGOUT("Registering devfs.\r\n");
    register_fs(&devfs_r);
}

inode_t* get_stdin_node(){
    return &devfs_stdin_node;
}
inode_t* get_stdout_node(){
    return &devfs_stdout_node;
}

inode_t* get_stderr_node(){
    return &devfs_stderr_node;
}

