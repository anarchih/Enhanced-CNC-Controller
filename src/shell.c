#include "FreeRTOS.h"
#include "task.h"
#include "host.h"

#include "shell.h"
#include <stddef.h>
#include "clib.h"
#include <string.h>
#include "fio.h"
#include "filesystem.h"


typedef struct {
	const char *name;
	cmdfunc *fptr;
	const char *desc;
} cmdlist;

void ls_command(int, char **);
void man_command(int, char **);
void cat_command(int, char **);
void ps_command(int, char **);
void host_command(int, char **);
void help_command(int, char **);
void host_command(int, char **);
void mmtest_command(int, char **);
void test_command(int, char **);
void test_ramfs_command(int, char **);

#define MKCL(n, d) {.name=#n, .fptr=n ## _command, .desc=d}

cmdlist cl[]={
	MKCL(ls, "List directory"),
	MKCL(man, "Show the manual of the command"),
	MKCL(cat, "Concatenate files and print on the stdout"),
	MKCL(ps, "Report a snapshot of the current processes"),
	MKCL(host, "Run command on host"),
	MKCL(mmtest, "heap memory allocation test"),
	MKCL(help, "help"),
	MKCL(test, "test new function"),
    MKCL(test_ramfs, "test ramfs"),
};

int parse_command(char *str, char *argv[]){
	int b_quote=0, b_dbquote=0;
	int i;
	int count=0, p=0;
	for(i=0; str[i]; ++i){
		if(str[i]=='\'')
			++b_quote;
		if(str[i]=='"')
			++b_dbquote;
		if(str[i]==' '&&b_quote%2==0&&b_dbquote%2==0){
			str[i]='\0';
			argv[count++]=&str[p];
			p=i+1;
		}
	}
	/* last one */
	argv[count++]=&str[p];

	return count;
}

void ls_command(int n, char *argv[]){
    uint8_t listFlag = 0;
    struct dir_entity ent;
    int dir, c;

    if(n == 1){
		fio_printf(2, "\r\nUsage: ls [-l] [path...]\r\n");
        return;
    }
    
    for(int i = 0; i < n; i++){
        if(argv[i][0] == '-'){
            if(strcmp(argv[i], "-l") == 0){
                listFlag = 1;
            }else{
                fio_printf(1, "\r\nUnsupported option %s\r\n", argv[i]);
                return;
            }
        }
    }
     
    for(int i = 1; i < n; i++){
        if(argv[i][0] != '-'){
            fio_printf(1, "\r\n");
            dir = fio_opendir(argv[i]); //Treat last argv as path
                        
            for(c = 0; fio_readdir(dir, &ent) >= 0; c++);
            fio_seekdir(dir, 0);

            fio_printf(1, "%s:\r\n", argv[i]); 
            fio_printf(1, "Total : %d\r\n", c);
            for(c = 0; fio_readdir(dir, &ent) >= 0; c++){
                if(!listFlag){
                    fio_printf(1, "%s", ent.d_name);
                    if(c > 5){
                       fio_printf(1, "\r\n");
                       c = -1;
                    }
                    else{
                        fio_printf(1, "\t");
                        c++;
                    }
                }else{

                    char attrs[11] = "----------\0";
                    if(ent.d_attr & 0x01)
                        attrs[0] = 'd';
                    fio_printf(1, "%s\t%s\r\n", attrs, ent.d_name);
                }
            }

            if(c != 0)
                fio_printf(1, "\r\n");
            
            fio_closedir(dir);
        }
    }
    return;
}

int filedump(const char* filename){
	char buf[128];

	int fd=fio_open(filename, 0, O_RDONLY);

	if(fd==OPENFAIL)
		return 0;

	fio_printf(1, "\r\n");

	int count;
	while((count=fio_read(fd, buf, sizeof(buf)))>0){
		fio_write(1, buf, count);
	}

	fio_close(fd);
	return 1;
}


void ps_command(int n, char *argv[]){
    /*
	char buf[1024];
	vTaskList(buf);
    fio_printf(1, "\n\rName          State   Priority  Stack  Num\n\r");
    fio_printf(1, "*******************************************\n\r");
	fio_printf(1, "%s\r\n", buf + 2);	
    */
}


void cat_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: cat <filename>\r\n");
		return;
	}

	if(filedump(argv[1]) < 0)
		fio_printf(2, "\r\n%s no such file or directory.\r\n", argv[1]);

    fio_printf(1, "\r\n");
}

void man_command(int n, char *argv[]){
	if(n==1){
		fio_printf(2, "\r\nUsage: man <command>\r\n");
		return;
	}

	char buf[128]="/romfs/manual/";
	strcat(buf, argv[1]);

	if(!filedump(buf))
		fio_printf(2, "\r\nManual not available.\r\n");
}

void host_command(int n, char *argv[]){
    int i, len = 0, rnt;
    char command[128] = {0};

    if(n>1){
        for(i = 1; i < n; i++) {
            memcpy(&command[len], argv[i], strlen(argv[i]));
            len += (strlen(argv[i]) + 1);
            command[len - 1] = ' ';
        }
        command[len - 1] = '\0';
        rnt=host_action(SYS_SYSTEM, command);
        fio_printf(1, "\r\nfinish with exit code %d.\r\n", rnt);
    } 
    else {
        fio_printf(2, "\r\nUsage: host 'command'\r\n");
    }
}

void help_command(int n,char *argv[]){
	int i;
	fio_printf(1, "\r\n");
	for(i=0;i<sizeof(cl)/sizeof(cl[0]); ++i){
		fio_printf(1, "%s - %s\r\n", cl[i].name, cl[i].desc);
	}
}

void test_command(int n, char *argv[]) {
    int handle;
    int error;

    fio_printf(1, "\r\n");

    handle = host_action(SYS_OPEN, "output/syslog", 8);
    if(handle == -1) {
        fio_printf(1, "Open file error!\n\r");
        return;
    }

    char *buffer = "Test host_write function which can write data to output/syslog\n";
    error = host_action(SYS_WRITE, handle, (void *)buffer, strlen(buffer));
    if(error != 0) {
        fio_printf(1, "Write file error! Remain %d bytes didn't write in the file.\n\r", error);
        host_action(SYS_CLOSE, handle);
        return;
    }

    host_action(SYS_CLOSE, handle);
}

void test_ramfs_command(int n, char *argv[]) {
    
    int file;
    char buf[16];

    fio_printf(1, "\r\n");

    fio_mkdir("/tmp/");
    
    file = fio_open("/tmp/test", 0, O_RDWR);

    fio_write(file, "TEST!\0", 6);
    
    fio_seek(file, 0, SEEK_SET);

    fio_read(file, buf, 6);

    fio_printf(1, "%s\r\n", buf);

    fio_close(file);

    return;
}

cmdfunc *do_command(const char *cmd){

	int i;

	for(i=0; i<sizeof(cl)/sizeof(cl[0]); ++i){
		if(strcmp(cl[i].name, cmd)==0)
			return cl[i].fptr;
	}
	return NULL;	
}
