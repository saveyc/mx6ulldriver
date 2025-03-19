#include "stdio.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "sys/ioctl.h"
#include "fcntl.h"
#include "stdlib.h"
#include "string.h"
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include "linux/ioctl.h"
#include <signal.h>
#include <fcntl.h>




int main(int argc, char* argv[])
{
    char* filename;
    int fd,ret;
    signed int databuf[7];
    unsigned char data[14];
    signed int gyro_x_adc,gyro_y_adc,gyro_z_adc;
    signed int accel_x_adc,accel_y_adc,accel_z_adc;
    signed int temp_adc;

    float gyro_x_act,gyro_y_act,gryo_z_act;
    float accel_x_act,accel_y_act,accel_z_act;
    float temp_act;



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

    while(1) {
        ret = read(fd,databuf,sizeof(databuf));
        if(ret == 0){
            gyro_x_adc = databuf[0];
            gyro_y_adc = databuf[1];
            gyro_z_adc = databuf[2];
            accel_x_adc = databuf[3];
            accel_y_adc = databuf[4];
            accel_z_adc = databuf[5];
            temp_adc = databuf[6];

            gyro_x_act = (float)gyro_x_adc / 16.4;
            gyro_y_act = (float)gyro_y_adc / 16.4;
            gryo_z_act = (float)gyro_z_adc / 16.4;

            accel_x_act = (float)accel_x_adc / 2048.0;
            accel_y_act = (float)accel_y_adc / 2048.0;
            accel_z_act = (float)accel_z_adc / 2048.0;

            temp_act = ((float)(temp_adc) -25) / 326.8 + 25;

            printf("\r\n 原始值： \r\n");
            printf("gyro_x_adc = %d, gyro_y_adc = %d, gyro_z_adc = %d\r\n",gyro_x_adc,gyro_y_adc,gyro_z_adc);
            printf("accel_x_adc = &d, accel_y_adc = %d, accel_z_adc = %d \r\n", accel_x_adc,accel_y_adc,accel_z_adc);
            printf("temp_adc = %d\r\n",temp_adc);
            printf("\r\n 转换值：\r\n");
            printf("gyro_x_act = %.2f °/s， gyro_y_act = %.2f °/s ，gyro_z_act = %.2f °/s" , gyro_x_act,gyro_y_act,gryo_z_act);
            printf("accel_x_act = %.2f °/s， accel_y_act = %.2f °/s ，accel_z_act = %.2f °/s" , accel_x_act,accel_y_act,accel_z_act);
            printf("temp_act = %.2f °/s",temp_act);
        }
        usleep(100000);
    }
    close(fd);
    return 0;


}
