#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/ioctl.h>

int main(int argc, char* argv[])
{
    int fd,ret;
    char* filename;
    unsigned char data = 0;

    if(argc != 2) {
        printf("Usage: %s <filename>\n",argv[0]);
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("open %s failed\n",filename);
        return -1;
    }

    while(1) {
        ret = read(fd, &data, 1);  
        if(ret < 0) {
            printf("read data failed\n");
        } 
        else{
            if(data){
                printf("data = %#x\r\n",data);
            }
        }

    }

    close(fd);  
    return 0;
}
