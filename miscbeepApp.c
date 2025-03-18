#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"
#include "linux/ioctl.h"
#include "signal.h"

#define BEEPOFF 0
#define BEEPON 1


int main(int argc, char* argv[])
{
    char* filename;
    int fd,retval;
    unsigned char databuf[1];



    if(argc != 3) {
        printf("Usage: %s <filename> 0 or 1\n",argv[0]);
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("open %s failed\n",filename);
        return -1;
    }

    databuf[0] = atoi(argv[2]);
    retval = write(fd, databuf, 1);
    if(retval < 0){
        printf("LED0 Control Failed \r\n");
        close(fd);
        return -1;
    }

    retval = close(fd);
    if(retval < 0){
        printf("file %s close failed \r\n",filename);
        return -1;
    }

    return 0;


}
