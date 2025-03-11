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
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>

#define  NEWCHRLED_CNT      1
#define  NEWCHRLED_NAME    "newchrled"
#define  LEDOFF             0
#define  LEDON              1

#define  CCM_CCGR1_BASE          (0x020C406C)
#define  SW_MUX_GPIO1_IO03_BASE  (0x020E0068)
#define  SW_PAD_GPIO1_IO03_BASE  (0x020E02F4)
#define  GPIO1_DR_BASE           (0x0209C000)
#define  GPIO1_GDIR_BASE         (0x0209C004)

static void __iomem*  IMX6U_CCM_CCGR1;
static void __iomem*  IMX6U_SW_MUX_GPIO1_IO03;
static void __iomem*  IMX6U_SW_PAD_GPIO1_IO03;
static void __iomem*  IMX6U_GPIO1_DR;
static void __iomem*  IMX6U_GPIO1_GDIR;

struct newchrdev_led{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
}

struct newchedev_led newchrled;
void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LEDON)
    {
        val = readl(IMX6U_GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, IMX6U_GPIO1_DR);
    }
    else if(sta == LEDOFF)
    {
        val = readl(IMX6U_GPIO1_DR);
        val |= (1 << 3);
        writel(val, IMX6U_GPIO1_DR);    
    }

}

static int led_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &newchrled;
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    int retval = 0;
    unsigned char databuf[1];
    unsigned char ledstat = 0;

    retval = copy_from_user(databuf, buf, count);
    if(retval < 0){
        printk("copy_from_user failed \r\n");
        return -EFAULT;
    }
    ledstat = databuf[0];

    if(ledstat == LEDON)
    {
        led_switch(ledstat);
    }
    else if(ledstat == LEDOFF)
    {
        led_switch(ledstat);
    }

    return 0;

}

static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
}; 

static int __init led_init(void)
{
    u32 val = 0;
    IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE,4);
    IMX6U_SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE,4);
    IMX6U_SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE,4);
    IMX6U_GPIO1_DR = ioremap(GPIO1_DR_BASE,4);
    IMX6U_GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE,4);

    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 10);
    val |= (3 << 10);
    writel(val, IMX6U_CCM_CCGR1);

    writel(5, IMX6U_SW_MUX_GPIO1_IO03);
    writel(0x10B0, IMX6U_SW_PAD_GPIO1_IO03);

    val = readl(IMX6U_GPIO1_GDIR);
    val &= ~(1 << 3);
    val |= (1 << 3);
    writel(val, IMX6U_GPIO1_GDIR);

    val = readl(IMX6U_GPIO1_DR);
    val |= (1 << 3);
    writel(val, IMX6U_GPIO1_DR);

    if(newchrled.major){
        newchrled.devid = MKDEV(newchrled.major, 0);
        register_chrdev_region(newchrled.devid, NEWCHRLED_CNT, NEWCHRLED_NAME);
    }
    else{
        alloc_chrdev_region(&newchrled.devid,0,NEWCHRLED_CNT,NEWCHRLED_NAME);
        newchrled.major = MAJOR(newchrled.devid);
        newchrled.minor = MINOR(newchrled.devid);
    }
     printk("major = %d, minor = %d \r\n", newchrled.major, newchrled.minor);

     newchrled.cdev.owner = THIS_MODULE;
     cdev_init(&newchrled.cdev, &led_fops);
     cdev_add(&newchrled.cdev, newchrled.devid, NEWCHRLED_CNT);

     newchrled.class = class_create(THIS_MODULE, "newchrled");
     if(IS_ERR(newchrled.class)){
         return PTR_ERR(newchrled.class);
     }

     newchrled.device = device_create(newchrled.class, NULL, newchrled.devid, NULL, NEWCHRLED_NAME);
     if(IS_ERR(newchrled.device)){
         return PTR_ERR(newchrled.device);         
     }

     return 0;
}

static void __exit led_exit(void)
{
    device_destroy(newchrled.class, newchrled.devid);
    class_destroy(newchrled.class);
    cdev_del(&newchrled.cdev);
    unregister_chrdev_region(newchrled.devid, NEWCHRLED_CNT);
    iounmap(IMX6U_CCM_CCGR1);
    iounmap(IMX6U_SW_MUX_GPIO1_IO03);
    iounmap(IMX6U_SW_PAD_GPIO1_IO03);
    iounmap(IMX6U_GPIO1_DR);
    iounmap(IMX6U_GPIO1_GDIR);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");



