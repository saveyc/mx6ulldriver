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
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>

#define IMX6UIRQ_CNT        1
#define IMX6UIRQ_NAME       "imx6uirq"
#define KEY_VALUE           0x01
#define KEY_INVALUE         0xFF
#define KEY_NUM             1

struct irq_keydesc{
    int gpio;
    int irqNum;
    unsigned char value;
    char name[10];
    irqreturn_t (*handler)(int irq, void *dev_id);
}

struct imx6uirq_dev{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *dev;
    struct device_node *node;
    int major;
    int minor;
    atomic_t keyValue;
    atomic_t keyRelease;
    int curKeyNum;
    struct timer_list timer;
    struct irq_keydesc keyDesc[KEY_NUM];
}

struct imx6uirq_dev imx6uirq;

static irqreturn_t key0_handler(int irq, void *dev_id)
{   
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)dev_id;
    dev->curKeyNum = 0;
    dev->timer.data = dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
    return IRQ_RETVAL(IRQ_HANDLED);
}

void timer_function(unsigned long arg)
{   
    unsigned char value;
    unsigned char num;
    struct irq_keydesc *keydesc;
    struct imx6uirq_dev *dev = (struct imx6uirq_dev *)arg;

    num = dev->curKeyNum;
    keydesc = &dev->keyDesc[num];
    value = gpio_get_value(keydesc->gpio);
    if(value == 0){
        atomic_set(&dev->keyValue, keydesc->value);
    }else{
        atomic_set(&dev->keyRelease, 1);
        atomic_set(&dev->keyValue, 0x80 | keydesc->value);
    }
}

static int keyio_init(void)
{   
    int ret = 0;
    unsigned char i = 0; 

    struct irq_keydesc *keydesc = imx6uirq.keyDesc;

    imx6uirq.node = of_find_node_by_path("/key");
    if(!imx6uirq.node == NULL){
        printk("find key node failed\n");
        return -EINVAL;
    }
    
    for(i=0;i<KEY_NUM;i++){
        imx6uirq.keyDesc[i].gpio = of_get_named_gpio(imx6uirq.node, "key-gpio", i);
        if(imx6uirq.keyDesc[i].gpio < 0){
            printk("get key%d gpio failed\n",i);
        }
    }
    for(i=0;i<KEY_NUM;i++){
        memset(&imx6uirq.keyDesc[i].name, 0, sizeof(imx6uirq.keyDesc[i].name));
        sprintf(imx6uirq.keyDesc[i].name, "key%d", i);
        gpio_request(imx6uirq.keyDesc[i].gpio, imx6uirq.keyDesc[i].name);
        gpio_direction_input(imx6uirq.keyDesc[i].gpio);
        imx6uirq.keyDesc[i].irqNum = irq_of_parse_and_map(imx6uirq.node, i);
    #if 0
        imx6uirq.keyDesc[i].irqNum = gpio_to_irq(imx6uirq.keyDesc[i].gpio);
    #endif
    }

    imx6uirq.keyDesc[0].handler = key0_handler;
    imx6uirq.keyDesc[0].value = KEY_VALUE;

    for(i=0;i<KEY_NUM;i++){
        ret = request_irq(imx6uirq.keyDesc[i].irqNum, imx6uirq.keyDesc[i].handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
            imx6uirq.keyDesc[i].name, &imx6uirq);
        if(ret < 0){
            printk("request key%d irq failed\n",imx6uirq.keyDesc[i].irqNum);
            return -EFAULT;
        }
    }

    init_timer(&imx6uirq.timer);
    imx6uirq.timer.function = timer_function;
    return 0;
}

static int imx6uirq_open(struct inode* inode, struct file* filp)
{
    filp->private_data = &imx6uirq;
    return 0;
    /* data */
}

static ssize_t imx6uirq_read(struct file* filp, char __user * buf, size_t cnt ,loff_t* offt)
{
    int ret;
    unsigned char keyvalue;
    unsigned char keyrelease;
    struct imx6uirq_dev *dev = filp->private_data;

    keyvalue = atomic_read(&imx6uirq.keyValue);
    keyrelease = atomic_read(&imx6uirq.keyRelease);

    if(keyrelease == 1){
        if(keyvalue & 0x80){
            keyvalue = keyvalue & 0x7f;
            ret =  copy_to_user(buf, &keyvalue, sizeof(keyvalue));
        }
        else{
            goto data_error;
        }

        atomic_set(&imx6uirq.keyRelease, 0);
    }
    else{
        got data error;
    }
    return 0;
data_error:
    return -EINVAL;
}

static struct file_operations imx6uirq_fops = {
    .owner = THIS_MODULE,
    .open = imx6uirq_open,
    .read = imx6uirq_read,
    .write = NULL,
    .release = NULL,
    .unlocked_ioctl = NULL,
};


static int __init imx6uirq_init(void)
{
    if(imx6uirq.major){
        imx6uirq.devid = MKDEV(imx6uirq.major, 0);
        register_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
    }
    else{
        imx6uirq.devid = alloc_chrdev_region(&imx6uirq.devid, 0, IMX6UIRQ_CNT, IMX6UIRQ_NAME);
        imx6uirq.major = MAJOR(imx6uirq.devid);
        imx6uirq.minor = MINOR(imx6uirq.devid);
    }

    imx6uirq.cdev.owner = THIS_MODULE;
    cdev_init(&imx6uirq.cdev, &imx6uirq_fops);
    cdev_add(&imx6uirq.cdev, imx6uirq.devid, IMX6UIRQ_CNT);

    imx6uirq.class = class_create(THIS_MODULE, IMX6UIRQ_NAME);
    if(IS_ERR(imx6uirq.class)){
        return PTR_ERR(imx6uirq.class);
    }
    
    imx6uirq.device = device_create(imx6uirq.class, NULL, imx6uirq.devid, NULL, IMX6UIRQ_NAME);
    if(IS_ERR(imx6uirq.device)){
        return PTR_ERR(imx6uirq.device);
    }

    atomic_set(&imx6uirq.keyValue, KEY_INVALUE);
    atomic_set(&imx6uirq.keyRelease, 0);
    keyio_init();

    return 0;

}

static void __exit imx6uirq_exit(void)
{
    unsigned int i;
    for(i=0;i<KEY_NUM;i++){
        free_irq(imx6uirq.keyDesc[i].irqNum, &imx6uirq);
    }

    del_timer(&imx6uirq.timer);

    device_destroy(imx6uirq.class, imx6uirq.devid);
    class_destroy(imx6uirq.class);
    cdev_del(&imx6uirq.cdev);
    unregister_chrdev_region(imx6uirq.devid, IMX6UIRQ_CNT);
}



module_init(imx6uirq_init);
module_exit(imx6uirq_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");



