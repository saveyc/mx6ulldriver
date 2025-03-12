#include "stdio.h"
#include "string.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"
#include "errno.h"
#include "stdlib.h"

#define BEEPOFF  0
#define BEEPON   1



int main(int argc, char *argv[]) 
{   
    int fd, retval;
    char *filename;
    unsigned char databuf[1];

    if(argc != 3) {
        printf("err usage \r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename, O_RDWR);

    if(fd < 0) {
        printf("open %s failed \r\n", filename);
        return -1;
    }

    databuf[0] = atoi(argv[2]);

    retval = write(fd, databuf, 1);
    if(retval < 0) {
        printf("kernel write failed \r\n");
        close(fd);
        return -1;
    }

    retval = close(fd);
    if(retval < 0) {
        printf("close %s failed \r\n", filename);
        return -1;
    }

    

    
    return 0;

}