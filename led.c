#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>


#define LED_MAJOR  200
#define LED_NAME   "led"

#define LED_OFF    0
#define LED_ON     1


#define CCM_CCGR1_BASE           (0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE   (0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE   (0x020E02F4)
#define GPIO1_DR_BASE            (0x0209C000)
#define GPIO1_GDIR_BASE          (0x0209C004)

static void __iomem*  IMX6U_CCM_CCGR1;
static void __iomem*  IMX6U_SW_MUX_GPIO1_IO03;
static void __iomem*  IMX6U_SW_PAD_GPIO1_IO03;
static void __iomem*  IMX6U_GPIO1_DR;
static void __iomem*  IMX6U_GPIO1_GDIR;

void led_switch(u8 sta)
{
    u32 val = 0;
    if(sta == LED_ON){
        val = readl(IMX6U_GPIO1_DR);
        val &= ~(1 << 3);
        writel(val, IMX6U_GPIO1_DR);
    }
    else if(sta == LED_OFF){
        val = readl(IMX6U_GPIO1_DR);
        val |= (1 << 3);
        writel(val, IMX6U_GPIO1_DR);
    }
}

static int led_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t led_read(struct file *filp, char __user *buf, size_t count, loff_t *pos)
{
    return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
    int ret = 0;
    unsigned char databuf[1];
    unsigned char ledstat;

    ret = copy_from_user(databuf,buf,count);
    if(ret < 0){
        printk("kernel write failed \r\n");
        return -EFAULT;
    }

    ledstat = databuf[0];

    if(ledstat == LED_ON){
        led_switch(LED_ON);
    }
    else if (ledstat == LED_OFF){
        led_switch(LED_OFF);
    }
    return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
    return 0;
}

static struct file_operations led_fops = {
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};

static int __init led_init(void)
{
    int ret = 0;
    u32 val =0;

    IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 0x4);
    IMX6U_SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 0x4);
    IMX6U_SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 0x4);
    IMX6U_GPIO1_DR = ioremap(GPIO1_DR_BASE, 0x4);
    IMX6U_GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 0x4);

    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3<<26);
    val |= (3<<26);
    writel(val, IMX6U_CCM_CCGR1);

    writel(0x5, IMX6U_SW_MUX_GPIO1_IO03);
    writel(0x10B0, IMX6U_SW_PAD_GPIO1_IO03);

    val = readl(IMX6U_GPIO1_GDIR);
    val &= ~(1<<3);
    val |= (1 << 3);
    writel(val, IMX6U_GPIO1_GDIR);

    val = readl(IMX6U_GPIO1_DR);
    val |= (1 << 3);
    writel(val, IMX6U_GPIO1_DR);

    ret = register_chrdev(LED_MAJOR, LED_NAME, &led_fops);
    if(ret < 0){
        printk("register chrdev failed \r\n");
        return -EIO;
    }
    

    return 0;
}

static void __exit led_exit(void)
{
    unregister_chrdev(LED_MAJOR, LED_NAME);
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



