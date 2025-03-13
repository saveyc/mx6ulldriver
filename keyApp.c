#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define KEY0VALUE    0xF0
#define KEY0INVAL    0x00

int main(int argc, char* argv[])
{
    int fd,retvalue;
    char* filename;
    unsigned char keyvalue = KEY0INVAL;

    if(argc != 2){
        printf("Error uasge ! \r\n");
        return -1;
    }

    filename = argv[1];
    fd = open(filename,O_RDWR);
    if(fd < 0){
        printf("file %s open failed \r\n",argv[1]);
        return -1;
    }

   while(1){
        read(fd,&keyvalue,sizeof(keyvalue));
        if(keyvalue == KEY0VALUE){
            printf("KEY0 PRESS ,value = %#x \r\n", keyvalue);
        }
   }
   retvalue = close(fd);

   if(retvalue < 0){
     printf("file %s close failed \r\n ", argv[1]);
     return -1;
   }
    return 0;

}
