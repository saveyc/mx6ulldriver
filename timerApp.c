#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <linux/ioctl.h>

#define CLOSE_CMD      (__IO(0xEF,0x01))
#define OPEN_CMD       (__IO(0xEF,0x02))
#define SETPERIOD_CMD  (__IO(0xEF,0x03))

int main(int argc, char* argv[])
{
    int fd,ret;
    char* filename;
    unsigned int cmd;
    unsigned int arg;
    unsigned char str[100];

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
        printf("Input CMD:\r\n");
        ret = scanf("%d", &cmd);
        if(ret != 1) {
            gets(str);
        }
        if(cmd == 1){
            cmd = CLOSE_CMD;
        }else if(cmd == 2){
            cmd = OPEN_CMD;
        }else if(cmd == 3){
            cmd = SETPERIOD_CMD;
            printf("Input timer period:\r\n");
            ret = scanf("%d", &arg);
            if(ret != 1) {
                gets(str);
            }
        }
        ioctl(fd, cmd, &arg);

    }
    close(fd);


}
