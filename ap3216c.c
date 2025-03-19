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
#include "ap3216creg.h"

#define  AP3216C_CNT    1
#define  AP3216C_NAME   "ap3216c"

struct ap3216c_dev{
    struct cdev cdev;
    dev_t devid;
    struct class *class;
    struct device *device;
    struct device_node *node;
    int major;
    void* private_data;
    unsigned short ir,als,ps;
}

static struct ap3216c_dev ap3216cDev;

static int ap3216c_read_regs(struct ap3216c_dev *dev,u8 reg,void* val,int len)
{
    int ret = 0;
    struct i2c_msg msg[2];
    struct i2c_client* client ((struct i2c_client*)dev->private_data);

    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].buf = &reg;
    msg[0].len = 1;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].buf = val;
    msg[1].len = len;

    ret = i2c_transfer(client->adapter,msg,2);

    if(ret == 2){
        ret = 0;
    }else{
        printk("ap3216c_read_regs error  ret = %d  reg= %d  len= %d \n", ret,reg,len);
        return -EREMOTEIO;
    }

    return ret;

}

static s32 ap3216c_write_regs(struct ap3216c_dev *dev,u8 reg,u8* buf,int len)
{
    u8 b[256];
    struct i2c_msg msg;
    struct i2c_client* client ((struct i2c_client*)dev->private_data);

    b[0] = reg;
    memcpy(&b[1],buf,len);
    msg.addr = client->addr;
    msg.flags = 0;
    msg.buf = b;
    msg.len = len + 1;
    return  i2c_transfer(client->adapter,&msg,1);
}

static unsigned char ap3216c_read_reg(struct ap3216c_dev *dev,u8 reg)
{
    u8 data = 0;
    ap3216c_read_regs(dev,reg,&data,1);
    return data;
}

static void ap3216c_write_reg(struct ap3216c_dev *dev,u8 reg,unsigned char data)
{   
    u8 buf = 0;
    buf = data;
    ap3216c_write_regs(dev,reg,&buf,1);
}

void ap3216c_readdata(struct ap3216c_dev *dev)
{
    unsigned char i = 0;
    unsigned char buf[6] = 0;
    for(i=0;i<6;i++){
        buf[i] = ap3216c_read_reg(dev,AP3216C_IRDATALOW + i);
    }
    if(buf[0] & 0x80){
        dev->ir = 0;
    }else{
        dev->ir = ((unsigned char)buf[1] << 2) | buf[0] & 0x03; 
    }
    dev->als = ((unsigned char)buf[3] << 8) | buf[2];

    if(dev->ps & 0x40){
        dev->ps = 0;
    }else{
        dev->ps = ((unsigned char)(buf[5] & 0x3F) << 4) | buf[4] & 0x0F;
    }

}


static int ap3216c_open(struct inode *inode,struct file *filp)
{   
    filp->private_data = &ap3216cDev;
    ap3216c_write_reg(&ap3216cDev,AP3216C_SYSTEMCONFIG,0x04);
    medelay(50);
    ap3216c_write_reg(&ap3216cDev,AP3216C_SYSTEMCONFIG,0x03);   
    return 0;
}

static ssize_t ap3216c_read(struct file *filp,char __user *buf,size_t count,loff_t *f_pos)
{
    short data[3] = {0};
    long err = 0;
    struct ap3216c_dev *dev = filp->private_data;

    ap3216c_readdata(&ap3216cDev);
    data[0] = ap3216cDev.ir;
    data[1] = ap3216cDev.als;
    data[2] = ap3216cDev.ps;
    err = copy_to_user(buf,data,sizeof(short)*3);

    return 0;
}

static int ap3216c_release(struct inode *inode,struct file *filp)
{
    return 0;
}

static const struct file_operations ap3216c_fops = {
    .owner = THIS_MODULE,
    .open = ap3216c_open,
    .read = ap3216c_read,
    .release = ap3216c_release,
};


static int ap3216c_probe(struct i2c_client *client,struct i2c_device_id *id)
{

    if(ap3216cDev.major){
        ap3216cDev.devid = MKDEV(ap3216cDev.major, 0);
        register_chrdev_region(ap3216cDev.devid, AP3216C_CNT,AP3216C_NAME);
    }else{
        alloc_chrdev_region(&ap3216cDev.devid, 0, AP3216C_CNT,AP3216C_NAME);
        ap3216cDev.major = MAJOR(ap3216cDev.devid);
    }
    
    ap3216cDev.cdev.owner = THIS_MODULE;
    cdev_init(&ap3216cDev.cdev);
    cdev_add(&ap3216cDev.cdev,ap3216cDev.devid,AP3216C_CNT);

    ap3216cDev.class = class_create(THIS_MODULE,AP3216C_NAME);
    if(IS_ERR(ap3216cDev.class)){
        return PTR_ERR(ap3216cDev.class);
    }
    ap3216cDev.device = device_create(ap3216cDev.class,NULL,ap3216cDev.devid,NULL,AP3216C_NAME);
    if (IS_ERR(ap3216cDev.device))
    {
        return PTR_ERR(ap3216cDev.device);
    }

    ap3216cDev.private_data = client;
    return 0;   
}

static int ap3216c_remove(struct i2c_client *client)
{
    device_destroy(ap3216cDev.class,ap3216cDev.devid);
    class_destroy(ap3216cDev.class);
    cdev_del(&ap3216cDev.cdev);
    unregister_chrdev_region(ap3216cDev.devid,AP3216C_CNT);
    return 0;
}


static const struct i2c_device_id ap3216c_id[] = {
    {"alientek,ap3216c",0},
    {}
};

static const struct of_device_id ap3216c_of_match[] = {
    {.compatible="alientek,ap3216c"},
    {}
};

static struct i2c_driver ap3216c_driver = {
    .driver = {
        .name = "ap3216c",
        .owner = THIS_MODULE,
        .of_match_table = ap3216c_of_match,
    },
    .probe = ap3216c_probe,
    .remove = ap3216c_remove,
    .id_table = ap3216c_id,

};


static int __init ap3216c_init(void)
{
    return i2c_add_driver(&ap3216c_driver);
}

static void __exit ap3216c_exit(void)
{
    i2c_del_driver(&ap3216c_driver);
}

module_init(ap3216c_init);
module_exit(ap3216c_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");

