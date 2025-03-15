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

static int fd 0;

static void sigio_signal_func(int signal) 
{
    int err = 0;
    unsigned char keyvalue;
    err = read(fd,&keyvalue,sizeof(keyvalue));

    if(err < 0) {
        printf("read error\n");
    }
    else{
        printf("key%d is pressed\n",keyvalue);
    }
    
}

int main(int argc, char* argv[])
{
    int flags =0;
    char* filename;



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

    signal(SIGIO,sigio_signal_func);
    fcntl(fd,F_SETDOWN,getpid());
    flags = fcntl(fd,F_GETFD);
    fcntl(fd,F_SETFL, flags | FASYNC);

    while(1){
        sleep(2);
    }
   
    
    close(fd);  
    return 0;
}
