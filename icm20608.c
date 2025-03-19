#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux.of_irq.h> 
#include <linux/wait.h>
#include <lilnux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <linux.platform_driver.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>
#include "icm20608reg.h"

#define ICM20608_CNT        1  
#define ICM20608_NAME       "icm20608"

struct  icm20608_dev {
    struct cdev cdev;
    struct class *class;
    struct device *device;
    dev_t devid;
    struct device_node *node;
    int major;
    void* private_data;
    int cs_gpio;
    signed int  gyro_x_adc;
    signed int  gyro_y_adc;
    signed int  gyro_z_adc;
    signed int  accel_x_adc;
    signed int  accel_y_adc;
    signed int  accel_z_adc;
    signed int  temp_adc;
}

static struct icm20608_dev icm20608Dev;

static int icm20608_read_regs(struct icm20608_dev* dev, u8 reg, void *buf, int len)
{
    int ret = -1;
    unsigned char txdata[1];
    unsigned char* rxdata ;
    struct spi_message m;
    struct spi_transfer* t;
    struct spi_device* spi = (struct spi_device*)dev->private_data;

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    if(!t){
        return -ENOMEM;
    }

    rxdata = kzalloc(sizeof(unsigned char)*len, GFP_KERNEL);
    if(!rxdata){
        goto out1;
    }

    txdata[0] = reg | 0x80;
    t->tx_buf = txdata;
    t->rx_buf = rxdata;
    t->len = len + 1;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    ret = spi_sync(spi, &m);
    if(ret){
        goto out2;
    }
    memcpy(buf, rxdata + 1, len);

out2:
    kfree(rxdata);
out1:
    kfree(t);

    return ret;

}

static s32 icm20608_write_regs(struct icm20608_dev* dev,u8 reg, u8* buf,u8 len)
{
    int ret = -1;
    unsigned char * txdata;
    struct spi_message m;
    struct spi_transfer* t;
    struct spi_device* spi = (struct spi_device*)dev->private_data;

    t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);
    if(!t){
        return -ENOMEM;
    }

    txdata = kzalloc(sizeof(unsigned char)*len, GFP_KERNEL);
    if(!txdata){
        goto out1;
    }

    *txdata = reg & 0x7f;
    memcpy(txdata + 1, buf, len);
    t->tx_buf = txdata;
    t->len = len + 1;
    spi_message_init(&m);
    spi_message_add_tail(t, &m);
    ret = spi_sync(spi, &m);
    if(ret){
        goto out2;
    }
out2:
    kfree(txdata);
out1:
    kfree(t);
    return ret;
 
}

static unsigned char icm20608_read_onereg(struct icm20608_dev* dev, u8 reg)
{
    u8 data = 0;
    icm20608_read_regs(dev, reg, &data, 1);
    return data;
};

static void icm20608_write_onereg(struct icm20608_dev* dev, u8 reg, u8 data)
{
    u8 buf = data;
    icm20608_write_regs(dev, reg, &buf, 1);
}

void icm20608_readdata(struct icm20608_dev* dev)
{
    unsigned char data[14] = {0};
    icm20608_read_regs(dev, ICM20_ACCEL_XOUT_H, data, 14);

    dev->accel_x_adc = (unsigned short)((data[0] << 8) | data[1]);
    dev->accel_y_adc = (unsigned short)((data[2] << 8) | data[3]);
    dev->accel_z_adc =  (unsigned short)((data[4] << 8) | data[5]);
    dev->temp_adc = (unsigned short)((data[6] << 8) | data[7]);
    dev->gyro_x_adc = (unsigned short)((data[8] << 8) | data[9]);
    dev->gyro_y_adc = (unsigned short)((data[10] << 8) | data[11]);
    dev->gyro_z_adc = (unsigned short)((data[12] << 8) | data[13]);
}

static int icm20608_open(struct inode* inode, struct file* filp)
{
    filp->private_data = &icm20608Dev;
    return 0;
}

static ssize_t  icm20608_read(struct file* filp, char __user* buf, size_t count, loff_t* f_pos)
{
    signed int data[7];
    long err = 0;
    struct icm20608_dev* dev = filp->private_data;
    icm20608_readdata(dev);
    data[0] = dev->gyro_x_adc;
    data[1] = dev->gyro_y_adc;
    data[2] = dev->gyro_z_adc;
    data[3] = dev->accel_x_adc;
    data[4] = dev->accel_y_adc;
    data[5] = dev->accel_z_adc;
    data[6] = dev->temp_adc;
    err = copy_to_user(buf,data,sizeof(data));
    return 0;
       
}

static int icm20608_release(struct inode* inode, struct file* filp)
{
    return 0;
}


static const struct file_operations icm20608_fops = {
    .owner = THIS_MODULE,
    .open = icm20608_open,
    .release = icm20608_release,
    .read = icm20608_read,
};

void icm20608_reginit(void)
{
    u8 value =0;
    icm20608_write_onereg(&icm20608Dev,ICM20_PWR_MGMT_1,0x80);
    mdelay(50);
    icm20608_write_onereg(&icm20608Dev,ICM20_PWR_MGMT_1,0x01);
    mdelay(50);

    value = icm20608_read_onereg(&icm20608Dev,ICM20_WHO_AM_I);
    printk("ICM20608 ID = %#x \r\n", value);

    icm20608_write_onereg(&icm20608Dev,ICM20_SMPLRT_DIV,0x00);
    icm20608_write_onereg(&icm20608Dev,ICM20_GYRO_CONFIG,0x18);
    icm20608_write_onereg(&icm20608Dev,ICM20_ACCEL_CONFIG,0x18);
    icm20608_write_onereg(&icm20608Dev,ICM20_CONFIG,0x04);
    icm20608_write_onereg(&icm20608Dev,ICM20_ACCEL_CONFIG2,0x04);
    icm20608_write_onereg(&icm20608Dev,ICM20_PWR_MGMT_2,0x00);
    icm20608_write_onereg(&icm20608Dev,ICM20_LP_MODE_CFG,0x00);
    icm20608_write_onereg(&icm20608Dev,ICM20_FIFO_EN,0x00);
}

static int icm20608_probe(struct spi_device* spi)
{
    if(icm20608Dev.major){
        icm20608Dev.devid = MKDEV(icm20608Dev.major,0);
        register_chrdev_region(icm20608Dev,ICM20608_CNT,ICM20608_NAME);
    }
    else{
       alloc_chrdev_region(&icm20608Dev,0,ICM20608_CNT,ICM20608_NAME);
       icm20608Dev.major = MAJOR(icm20608Dev.devid);
       icm20608Dev.minor = MINOR(icm20608Dev.devid);
    }

    icm20608Dev.cdev.owner = THIS_MODULE;
    cdev_init(&icm20608Dev.cdev,&icm20608_fops);
    cdev_add(&icm20608Dev.cdev,icm20608Dev.devid,ICM20608_CNT);

    icm20608Dev.class = class_create(THIS_MODULE,ICM20608_NAME);
    if(IS_ERR(icm20608Dev.class)){
        return PTR_ERR(icm20608Dev.class);
    }

    icm20608Dev.device = device_create(icm20608Dev.class,NULL,icm20608Dev.devid,NULL,ICM20608_NAME);
    if(IS_ERR(icm20608Dev.device)){
        return PTR_ERR(icm20608Dev.device);
    }   

    spi->mode = SPI_MODE_0;
    spi_setup(spi);
    icm20608Dev.private_data = spi;
    icm20608_reginit();
    return 0;     
};

static int icm20608_remove(struct spi_device* spi)
{    
    device_destroy(icm20608Dev.class,icm20608Dev.devid);
    class_destroy(icm20608Dev.class);
    unregister_chrdev_region(icm20608Dev,ICM20608_CNT);
    cdev_del(&icm20608Dev.cdev);
    return 0;
}

static const struct spi_device_id icm20608_id[] = {
    {"alientek,icm20608",0},
    {}
};
static const struct of_device_id icm20608_of_match[] = {
    {.compatible = "alientek,icm20608",},
    {},
};

static struct spi_driver icm20608_driver = {
    .probe = icm20608_probe,
    .remove = icm20608_remove,
    .driver = {
        .name = "alientek,icm20608",
        .of_match_table = icm20608_of_match,
        .owner = THIS_MODULE,
    },
    .id_table = icm20608_id,
};

static int __init icm20608_init(void)
{
    return spi_register_driver(&icm20608_driver);
}

static void __exit icm20608_exit(void)
{
    return spi_unregister_driver(&icm20608_driver);
}

module_init(icm20608_init);
module_exit(icm20608_exit);







MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");

