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
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>

#define  BEEP_CNT    1
#define  BEEP_NAME   "beep"
#define  BEEPOFF      0
#define  BEEPON       1

struct beep_dev {
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int beepGpio;
    struct device_node *node;
    dev_t devid;
};

static struct beep_dev beep;

static int beep_open(struct inode *inode, struct file *filp){
    filp->private_data = &beep;
    return 0;
}

static ssize_t beep_read(struct file *filp, char __user *buf, size_t len, loff_t *off)
{
    return 0;
}

static ssize_t beep_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    int ret;
    unsigned char databuf[1];
    unsigned char beepstat = 0;
    struct beep_dev *dev = filp->private_data;

    ret = copy_from_user(databuf, buf, len);
    if(ret < 0) {
        printk("copy_from_user failed\r\n");
        return -EFAULT;
    }

    beepstat = databuf[0];
    if(beepstat == BEEPOFF) {
        gpio_set_value(beep.beepGpio, 1);
    } else if(beepstat == BEEPON) {
        gpio_set_value(beep.beepGpio, 0);
    }else{
        printk("beep control error\r\n");
    }
    
    return 0;
    
}

static int beep_release(struct inode *inode, struct file *filp){
    return 0;
}

static struct file_operations beep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .read = beep_read,
    .write = beep_write,
    .release = beep_release,
};

static int __init beep_init(void) {
    int ret;

    beep.node = of_find_node_by_path("/beep");
    if(beep.node == NULL) {
        printk("beep node not found\r\n");
        return -EINVAL;
    }else{
        printk("beep node found\r\n");
    }

    beep.beepGpio = of_get_named_gpio(beep.node, "beep-gpio", 0);
    if(beep.beepGpio < 0) {
        printk("beep gpio not found\r\n");
        return -EINVAL;
    }else{
        printk("beep gpio found\r\n");
    }

    ret = gpio_getdirection_output(beep.beepGpio,1);

    if(ret < 0) {
        printk("beep gpio set direction failed\r\n");
        return -EINVAL;
    }

    if(beep.major) {
        beep.devid = MKDEV(beep.major, 0);
        register_chrdev_region(beep.devid, BEEP_CNT, BEEP_NAME);
    } else {
        alloc_chrdev_region(&beep.devid, 0, BEEP_CNT, BEEP_NAME);
        beep.major = MAJOR(beep.devid);
        beep.minor = MINOR(beep.devid)
    }

    beep.cdev.owner = THIS_MODULE;
    cdev_init(&beep.cdev, &beep_fops);
    cdev_add(&beep.cdev, beep.devid, BEEP_CNT);

    beep.class = class_create(THIS_MODULE, BEEP_NAME);
    if(IS_ERR(beep.class)) {
        return PTR_ERR(beep.class);
    }

    beep.device = device_create(beep.class, NULL, beep.devid, NULL, BEEP_NAME);
    if(IS_ERR(beep.device)) {
        return PTR_ERR(beep.device);
    }    

    return 0;
}

static void __exit beep_exit(void) 
{
    device_destroy(beep.class, beep.devid);
    class_destroy(beep.class);
    cdev_del(&beep.cdev);
    unregister_chrdev_region(beep.devid, BEEP_CNT);
}

module_init(beep_init);
module_exit(beep_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");



