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
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>

#define  GPIOLED_CNT   1
#define  GPIOLED_NAME  "gpioled"
#define  LEDOFF        0
#define  LEDON         1

struct gpioled_dev{
    struct cdev cdev;
    struct class *class;
    struct device *dev;
    dev_t devid;
    int major;
    int minor;
    int gpio;
    struct device_node *node;
    struct semaphore sem;
};

struct gpioled_dev gpioled;

static int led_open(struct inode *inode, struct file *filp)
{

    unsigned long flags;
    filp->private_data = &gpioled;

   if(down_interruptible(&gpioled.sem)){
       return -ERESTARTSYS;
   }


    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int ret = 0;
    unsigned char databuf[1];
    unsigned char ledstat;
    struct gpioled_dev *dev = filp->private_data;

    ret = copy_from_user(databuf, buf, count);
    if(ret < 0) {
        printk("copy_from_user failed \r\n");
        return -EFAULT;
    }

    ledstat = databuf[0];
    if(LEDOFF == ledstat) {
        gpio_set_value(dev->gpio, 0);
    } else if(LEDON == ledstat) {
        gpio_set_value(dev->gpio, 1);
    } else {
        return -EINVAL;
    }

    return ret;
}

static int led_release(struct inode *inode, struct file *filp)
{
    up(&gpioled.sem);

    return 0;
}   

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,   
    .write = led_write,
    .release = led_release,
};    

static int __init led_init(void)
{
    int ret = 0;
    int gpio = 0;

    sema_init(&gpioled.sem, 1);

    gpioled.node = of_find_node_by_path("/gpioled");
    if(gpioled.node == NULL) {
        printk("can't find gpioled node \r\n");
        return -EINVAL;
    }
    else{
        printk("find gpioled node \r\n");
    }

    gpioled.gpio = of_get_named_gpio(gpioled.node, "gpio", 0);
    if(gpioled.gpio < 0) {
        printk("can't find gpioled gpio \r\n");
        return -EINVAL;
    }
    printk("gpioled gpio = %d \r\n", gpioled.gpio);

    ret = gpio_direction_output(gpioled.gpio, 0);
    if(ret < 0) {
        printk("gpio_direction_output failed \r\n");
    }

    if(gpioled.major) {
        gpioled.devid = MKDEV(gpioled.major, 0);
        register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
    } else {
        alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
    }
    
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev, &fops);
    cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

    gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
    if(IS_ERR(gpioled.class)) {
        ret = PTR_ERR(gpioled.class);
        return ret;
    }

    gpioled.device = device_create(gpioled.class,NULL,gpioled.devid,NULL,GPIOLED_NAME);
    if(IS_ERR(gpioled.device)) {
        ret = PTR_ERR(gpioled.device);
        return ret;
    }

    return 0;
}

static void __exit led_exit(void)
{
    device_destroy(gpioled.class, gpioled.devid);
    class_destroy(gpioled.class);
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);
}


module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");



