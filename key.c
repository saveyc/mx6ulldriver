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

#define KEY_CNT   1
#define KEY_NAME  "key"

#define KEY0VALUE  0xF0
#define KEY0INVA   0x00

struct key_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    int key_gpio;
    struct device_node *node = NULL;
    atomic_t key_value;
} 

struct key_dev keydev;

static int keyio_init(void)
{
    keydev.node = of_find_node_by_path("/gpio_keys");
    if(keydev.node == NULL){
        printk("key node not found \r\n");
        return -EINVAL;
    }
    keydev.key_gpio = of_get_named_gpio(keydev.node,"gpio-key",0);
    if(keydev.key_gpio < 0){
        printk("key gpio not found \r\n");
        return -EINVAL:
    }
    printk("key_gpio = %d \r\n",keydev.key_gpio);

    gpio_request(keydev.key_gpio,"key0");
    gpio_direction_input(keydev.key_gpio);
    return 0;
}

static int keyio_open(struct inode *inode,struct file *filp)
{
    int ret;

    filp->private_data = &keydev;
    ret = keyio_init();
    if(ret < 0){
        return ret; 
    }

    return 0;
}

static ssize_t keyio_read(struct file *filp,char *buf,size_t count,loff_t *f_pos)
{
    int ret;
    unsigned char value;
    struct key_dev *dev = filp->private_data;

    if(gpio_get_value(dev->key_gpio) == 0){
        while(gpio_get_value(dev->key_gpio) == 0);
        atomic_set(&dev->key_value,KEY0VALUE);
    }
    else{
        atomic_set(&dev->key_value,KEY0INVA);
    }

    value = atomic_read(&dev->key_value);
    ret = copy_to_user(buf,&value,sizeof(value));

    return ret;
}

static ssize_t keyio_write(struct file *filp,const char *buf,size_t count,loff_t *f_pos)
{
    return 0;
}

static int keyio_release(struct inode *inode,struct file *filp)
{
    return 0;
}   

struct file_operations keyio_fops = {
    .owner = THIS_MODULE,
    .open = keyio_open,
    .read = keyio_read,
    .write = keyio_write,
    .release = keyio_release,
};

static int __init keyio_init(void)
{
    atomic_init(&keydev.key_value,KEY0INVA);

    if(keydev.major){
        keydev.devid = MKDEV(keydev.major,keydev.minor);
        register_chrdev_region(keydev.devid,KEY_CNT,KEY_NAME);
    }
    else{
        alloc_chrdev_region(&keydev.devid,0,KEY_CNT,KEY_NAME);
        keydev.major = MAJOR(keydev.devid);
        keydev.minor = MINOR(keydev.devid);
    }
    printk("key major = %d minor = %d \r\n",keydev.major,keydev.minor);

    keydev.cdev.owner = THIS_MODULE;
    cdev_init(&keydev.cdev,&keyio_fops);
    cdev_add(&keydev.cdev,keydev.devid,KEY_CNT);

    keydev.class = class_create(THIS_MODULE,KEY_NAME);
    if(IS_ERR(keydev.class)){
        return PTR_ERR(keydev.class);
    }
    keydev.device = device_create(keydev.class,NULL,keydev.devid,NULL,KEY_NAME);
    if(IS_ERR(keydev.device)){
        return PTR_ERR(keydev.device);
    }

    return 0;
   
}

/**
 * keyio_exit - Exit function for the key I/O module.
 *
 * This function cleans up the resources allocated during the module's
 * initialization, such as unregistering the character device, destroying
 * the device and class, and releasing any GPIOs used.
 */

static void __exit keyio_exit(void)
{   
    device_destroy(keydev.class,keydev.devid);
    class_destroy(kerdev.class);
    cdev_del(&keydev.cdev);
    unregister_chrdev_region(keydev.devid,KEY_CNT);
    
}

module_init(keyio_init);
module_exit(keyio_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");



