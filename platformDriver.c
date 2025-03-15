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
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>

#define  LEDDEV_CNT         1
#define  LEDDEV_NAME        "platformled"
#define  LEDOFF             0
#define  LEDON              1

static void __iomem*        IMX6U_CCM_CCGR1;
static void __iomem*        IMX6U_SW_MUX_GPIO1_IO03;
static void __iomem*        IMX6U_SW_PAD_GPIO1_IO03;
static void __iomem*        IMX6U_GPIO1_DR;
static void __iomem*        IMX6U_GPIO1_GDIR;


struct leddev_dev{
    dev_t devid;
    struct cdev cdev;
    struct class* class;
    struct device* device;
    int major;
    int minor;
};

struct leddev_dev   ledDev;

static void led0_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDOFF){
        val = readl(IMX6U_GPIO1_DR);
        val |= (1 << 3);
        writel(val, IMX6U_GPIO1_DR);
    }
    else if(sta == LEDON){
        val = readl(IMX6U_GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, IMX6U_GPIO1_DR);
    }

}

static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &ledDev;
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t len, loff_t *off)
{
    int ret = 0;
    unsigned char sta = 0;
    unsigned char databuf[1];

    ret = copy_from_user(databuf, buf, len);
    if(ret < 0){
        -EFAULT;
    }
    sta = databuf[0];

    if(sta == LEDOFF){
        led0_switch(LEDOFF);
    }
    else if(sta == LEDON){
        led0_switch(LEDON);
    }

    return 0;  
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
};


static int led_probe(struct platform_device *pdev)
{   
    int i=0;
    u32 val = 0;
    int ressize[5];
    struct resource *res[5];

    printk("led probe\r\n");
    
    for(i=0;i<5;i++){   
        res[i] = platform_get_resource(pdev,IORESOURCE_MEM,i);
        if(res[i] == NULL){
            dev_err(&pdev->dev, "failed to get resource %d\n", i);
            return -ENXIO;
        }
        ressize[i] = resource_size(res[i]);
    }

    IMX6U_CCM_CCGR1 = ioremap(res[0]->start, ressize[0]);
    IMX6U_SW_MUX_GPIO1_IO03 = ioremap(res[1]->start, ressize[1]);
    IMX6U_SW_PAD_GPIO1_IO03 = ioremap(res[2]->start, ressize[2]);
    IMX6U_GPIO1_DR = ioremap(res[3]->start, ressize[3]);
    IMX6U_GPIO1_GDIR = ioremap(res[4]->start, ressize[4]);


    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(0x3 << 26);
    val |= (3 << 26);
    writel(val, IMX6U_CCM_CCGR1);

    writel(0x5, IMX6U_SW_MUX_GPIO1_IO03);
    writel(0x10B0, IMX6U_SW_PAD_GPIO1_IO03);

    val = readl(IMX6U_GPIO1_GDIR);
    val &= ~(1 << 3);
    val |= (1 << 3);
    writel(val, IMX6U_GPIO1_GDIR);

    val = readl(IMX6U_GPIO1_DR);
    val |= (1 << 3);
    writel(val, IMX6U_GPIO1_DR);

    if(ledDev.major){
        ledDev.devid = MKDEV(ledDev.major, 0);
        register_chrdev_region(ledDev.devid, LEDDEV_CNT, LEDDEV_NAME);
    }else{
        alloc_chrdev_region(&ledDev.devid, 0, LEDDEV_CNT, LEDDEV_NAME);
        ledDev.major = MAJOR(ledDev.devid);
        ledDev.minor = MINOR(ledDev.devid);
    }

    ledDev.cdev.owner = THIS_MODULE;
    cdev_init(&ledDev.cdev);
    cdev_add(&ledDev.cdev, ledDev.devid, LEDDEV_CNT);

    ledDev.class = class_create(THIS_MODULE, LEDDEV_NAME);
    if(IS_ERR(ledDev.class)){
        return PTR_ERR(ledDev.class);
    }

    ledDev.device = device_create(ledDev.class, NULL, ledDev.devid, NULL, LEDDEV_NAME);
    if(IS_ERR(ledDev.device)){
        return PTR_ERR(ledDev.device);
    }

    return 0;
}

static int led_remove(struct platform_device *pdev)
{
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(IMX6U_SW_MUX_GPIO1_IO03);
    iounmap(IMX6U_SW_PAD_GPIO1_IO03);
    iounmap(IMX6U_GPIO1_DR);
    iounmap(IMX6U_GPIO1_GDIR);

    cdev_del(&ledDev.cdev);
    class_destroy(ledDev.class);
    device_destroy(ledDev.class, ledDev.devid);
    unregister_chrdev_region(ledDev.devid, LEDDEV_CNT);
    return 0;
}

static struct platform_driver led_driver = {
    .probe = led_probe,
    .remove = led_remove,
    .driver = {
        .name = "imx6u-led",
        .owner = THIS_MODULE,
    },
};

static int __init led_init(void)
{
    return platform_driver_register(&led_driver);
}

static void __exit led_exit(void)
{   
    platform_driver_unregister(&led_driver);
}

module_init(led_init);
module_exit(led_exit);  

MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");



