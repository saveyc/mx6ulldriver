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
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <linux.platform_driver.h>
#include <linux/miscdevice.h>
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>


#define MISCBEEP_MINOR 144
#define MISCBEEP_NAME "miscbeep"
#define BEEPOFF 0   
#define BEEPON 1

struct miscbeep_dev {
    struct cdev cdev;
    struct class *class;
    struct device *dev;
    dev_t devid;
    struct device_node *node;
    int beep_gpio;
};

struct miscbeep_dev miscbeep;

static int miscbeep_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &miscbeep;
    return 0;
}

static ssize_t miscbeep_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int ret;
    unsigned char databuf[1];
    unsigned char sta;
    struct miscbeep_dev *dev= filp->private_data;

    ret = copy_from_user(databuf, buf, count));
    if(ret < 0){
        printk("copy_from_user error\n");
        return -EFAULT;
    }
    sta = databuf[0];

    if(sta == BEEPOFF){
        gpio_set_value(miscbeep.beep_gpio, 1);
    }else if(sta == BEEPON){
        gpio_set_value(miscbeep.beep_gpio, 0);
    }

    return 0;
}

static struct file_operations miscbeep_fops = {
    .open = miscbeep_open,
    .write = miscbeep_write,
    .owner = THIS_MODULE,
};

static struct miscdevice beep_miscdev = {
    .minor = MISCBEEP_MINOR,
    .name = MISCBEEP_NAME,
    .fops = &miscbeep_fops,
}

static int miscbeep_probe(struct platform_device *pdev)
{
    int ret;
    printk("miscbeep_probe\n");

    miscbeep.node = of_find_node_by_path("/miscbeep");
    if(miscbeep.node == NULL){
        printk("miscbeep node not found\n");
        return -EINVAL;
    }

    miscbeep.beep_gpio = of_get_named_gpio(miscbeep.node, "beep-gpio", 0);
    if(miscbeep.beep_gpio < 0){
        printk("miscbeep gpio not found\n");
        return -EINVAL;
    }

    ret = gpio_direction_output(miscbeep.beep_gpio, 1);
    if(ret < 0){
        printk("miscbeep gpio direction error\n");
    }

    ret = misc_register(&beep_miscdev);
    if(ret < 0){
        printk("miscbeep register error\n");
        return EFAULT;
    }

    return 0;
}

static int miscbeep_remove(struct platform_device *pdev)
{
    gpio_set_value(miscbeep.beep_gpio, 1);
    misc_deregister(&beep_miscdev);
    return 0;
}

static const struct of_device_id miscbeep_of_match[] = {
    { .compatible = "miscbeep", },
    { /* sentinel */ }
};

static struct platform_driver miscbeep_driver = {   
    .probe = miscbeep_probe,
    .remove = miscbeep_remove,
    .driver = {
        .name = "miscbeep",
        .of_match_table = miscbeep_of_match,
    },
};

static int __init miscbeep_init(void)
{
    return platform_driver_register(&miscbeep_driver);
}

static void __exit miscbeep_exit(void)
{
    platform_dirver_unregister(&miscbeep_driver);
}

module_init(miscbeep_init);
module_exit(miscbeep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");







