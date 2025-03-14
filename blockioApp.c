#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "poll.h"
#include "sys/select.h"
#include "sys/time.h"
#include <linux/ioctl.h>

int main(int argc, char* argv[])
{
    int fd,ret;
    char* filename;
    struct pollfd fds;
    fd_set readfds;
    struct timeval tv;
    unsigned char data;


    if(argc != 2) {
        printf("Usage: %s <filename>\n",argv[0]);
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR | O_NONBLOCK);
    if(fd < 0) {
        printf("open %s failed\n",filename);
        return -1;
    }

    while(1) {
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        ret = select(fd+1, &readfds, NULL, NULL, &tv);
        switch(ret) {
            case -1:
                printf("select failed");
                break;
            case 0:
                printf("timeout\n");
                break;
            default:
                if(FD_ISSET(fd, &readfds)) {
                    ret = read(fd, &data, 1);
                    if(ret < 0) {
                        printf("read failed\n");
                    }else{  
                        if(data == 1) {
                            printf("key%d is pressed\n",data); 
                        }
                    }
                }
                break;
        }
    }
    close(fd);  
    return 0;
}
