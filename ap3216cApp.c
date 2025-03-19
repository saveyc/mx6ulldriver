#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <select.h>
#include <sys/time.h>
#include "linux/ioctl.h"
#include <signal.h>
#include <fcntl.h>




int main(int argc, char* argv[])
{
    char* filename;
    int fd,ret;
    unsigned short databuf[3];
    unsigned short ir,als,ps;



    if(argc != 2) {
        printf("Usage: %s <filename> 0 or 1\n",argv[0]);
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("open %s failed\n",filename);
        return -1;
    }

    while(1){
        ret = read(fd,&databuf,sizeof(databuf));
        if(ret == 0){
            ir = databuf[0];
            als = databuf[1];
            ps = databuf[2];
            printf("ir = %d,als = %d,ps = %d\n",ir,als,ps);
        }
        usleep(200000)
    }

    close(fd);
    return 0;


}
