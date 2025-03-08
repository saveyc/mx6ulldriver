#include "stdio.h"
#include "string.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"
#include "errno.h"
#include "stdlib.h"

static char usrdata[] = {"usr data"};

int main(int argc, char *argv[]) 
{   
    int fd, retval;
    char *filename;
    char readbuf[100],writebuf[100];

    if(argc !=3)
    {
        printf("Usage error: %s <filename> <data>\n", argv[0]);
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);

    if(fd < 0){
        printf("open error: %s\r\n", argv[1]);
    }

    if(atoi(argv[2]) == 1){
        retval = read(fd, readbuf,50);
        if(retval < 0){
            printf("read error: %s\r\n", argv[1]);
        }
        else{
            printf("read: %s\r\n", readbuf);
        }

    }

    if(atoi(argv[2]) == 2){

        memccpy(writebuf, usrdata, sizeof(usrdata));
        retval = write(fd, writebuf, 50);
        if(retval < 0){
            printf("write error: %s\r\n", argv[1]);
        }
        else{
            printf("write: %s\r\n", usrdata);            
        }
    }

    retval = close(fd);
    if(retval < 0){
        printf("close error: %s\r\n", argv[1]);
        return -1;
    }

    return 0;

}