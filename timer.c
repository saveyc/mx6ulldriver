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
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>

#define  TIMER_CNT      1
#define  TIMER_NAME     "timer"
#define  CLOSE_CMD      (__IO(0xEF,0x01))
#define  OPEN_CMD       (__IO(0xEF,0x02))
#define  SETPERIOD_CMD  (__IO(0xEF,0x03))

#define  LED_ON         1
#define  LED_OFF        0

struct timer_device {
    dev_t devid;
    struct cdev cdev;
    struct device *dev;
    struct class *class;
    struct device_node *node;
    int major;
    int minor;
    int ledGpio;
    int period;
    struct timer_list timer;
    struct spinlock lock;
};

static struct timer_device timerDev;

static int led_init(void)
{
    int ret = 0;

    timerDev.node = of_find_node_by_path("/leds/led1");
    if(timerDev.node == NULL) {
        printk("can't find led1 node\r\n");
        return -1;
    }

    timerDev.ledGpio = of_get_named_gpio(timerDev.node, "sys-led", 0);
    if(timerDev.ledGpio < 0) {
        printk("can't find sys-led gpio\r\n");
        return -1;
    }  
    gpio_request(timerDev.ledGpio, "sys-led");
    gpio_direction_output(timerDev.ledGpio, 1);
    
    return 0;
}   

static int timer_open(struct inode *inode, struct file *filp)
{
    int ret = 0;
    filp->private_data = &timerDev;
    timerDev.period = 1000;
    ret = led_init();
    if(ret < 0) {
        return ret;
    }
    return 0;
}

static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int timerPeriod = 0;
    unsigned long flag = 0;
    struct timer_device *dev = (struct timer_device *)filp->private_data;

    switch(cmd) {
        case CLOSE_CMD:
            del_timer_sync(&dev->timer);
        break;
        case OPEN_CMD:
            spin_lock_irqsave(&dev->lock, flag);
            timerPeriod = dev->period;
            spin_unlock_irqrestore(&dev->lock, flag);
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(timerPeriod));
        break;
        case SETPERIOD_CMD:
            spin_lock_irqsave(&dev->lock, flag);
            dev->period  = arg;
            spin_unlock_irqrestore(&dev->lock, flag);
            mod_timer(&dev->timer, jiffies + msecs_to_jiffies(arg));
        break;
        default:
        break;
    }

    return 0;   
}

static struct file_operations timer_fops = {
    .owner = THIS_MODULE,   
    .open = timer_open,
    .unlocked_ioctl = timer_unlocked_ioctl,
};

static void time_function(unsigned long arg)
{
    struct timer_device *dev = (struct timer_device *)arg;
    unsigned long flag = 0;
    static int sta = 0;
    int periodtime = 0;

    sta != sta;
    gpio_set_value(dev->ledGpio, sta);

    spin_lock_irqsave(&dev->lock, flag);
    periodtime = dev->period;
    spin_unlock_irqrestore(&dev->lock, flag);
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(periodtime));
}

static int __init timer_init(void)
{
    spin_lock_init(&timerDev.lock);

    if(timerDev.major) {
        timerDev.devid = MKDEV(timerDev.major, timerDev.minor);
        register_chrdev_region(timerDev.devid, TIMER_CNT, TIMER_NAME);
    } else {
        alloc_chrdev_region(&timerDev.devid, timerDev.minor, TIMER_CNT, TIMER_NAME);
        timerDev.major = MAJOR(timerDev.devid);
        timerDev.minor = MINOR(timerDev.devid);
    }

    timerDev.cdev.owner = THIS_MODULE;
    cdev_init(&timerDev.cdev, &timer_fops);
    cdev_add(&timerDev.cdev, timerDev.devid, TIMER_CNT);

    timerDev.class = class_create(THIS_MODULE, TIMER_NAME);
    if(IS_ERR(timerDev.class)) {
        return PTR_ERR(timerDev.class);
    }

    timerDev.dev = device_create(timerDev.class, NULL, timerDev.devid, NULL, TIMER_NAME);
    if(IS_ERR(timerDev.dev)) {
        return PTR_ERR(timerDev.dev);
    }

    init_timer(&timerDev.timer);
    timerDev.timer.function = time_function;
    timerDev.timer.data = (unsigned long)&timerDev;

    return 0;

}

static void __exit timer_exit(void)
{
    gpio_set_value(timerDev.ledGpio, 1);
    del_timer_sync(&timerDev.timer);

    device_destroy(timerDev.class, timerDev.devid);
    class_destroy(timerDev.class);
    cdev_del(&timerDev.cdev);
    unregister_chrdev_region(timerDev.devid, TIMER_CNT);
}


module_init(timer_init);
module_exit(timer_exit);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");



