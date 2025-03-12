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
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>

#define  NEWCHRLED_CNT      1
#define  NEWCHRLED_NAME    "dtsled"
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
    struct device_node *node;
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
    int ret = 0;
    u32 regdata[14];
    const char* str;
    struct property *proper;

    newchrled.node = of_find_node_by_path("/alphaled");
    if(newchrled.node == NULL){
        printk("alphaled node can not find \r\n");
        return -EINVAL;
    }
    else{
        printk("alphaled node has been found \r\n");
    }

    proper = of_find_property(newchrled.node,"compitable",NULL);
    if(proper == NULL){
        printk("compatible property find failed \r\n");
    }
    else{
        printk("compatible = %s\r\n",(char*)proper->value);
    }

    ret = of_property_read_string(newchrled.node,"status",&str);
    if(ret < 0){
        printk("status read failed \r\n");
    }
    else{
        printk("status = %s \r\n",str);
    }

    ret = of_property_read_u32_array(newchrled.node,"reg",regdata,10);
    if(ret < 0){   
        printk("reg property read failed \r\n");
    }
    else{
        u8 i =0;
        printk("regdata:\r\n");
        for(i=0;i<10;i++){
            printk("%#x",regdata[i]);
        }
        printk("\r\n");
    }

#if 0
    IMX6U_CCM_CCGR1 = ioremap(regdata[0],regdata[1]);
    IMX6U_SW_MUX_GPIO1_IO03 = ioremap(regdata[2],regdata[3]);
    IMX6U_SW_PAD_GPIO1_IO03 = ioremap(regdata[4],regdata[5]);
    IMX6U_GPIO1_DR = ioremap(regdata[6],regdata[7]);
    IMX6U_GPIO1_GDIR = ioremap(regdata[8],regdata[9]);
#else
    IMX6U_CCM_CCGR1 = of_iomap(newchrled.node,0);
    IMX6U_SW_MUX_GPIO1_IO03 = of_iomap(newchrled.node,1);
    IMX6U_SW_PAD_GPIO1_IO03 = of_iomap(newchrled.node,2);
    IMX6U_GPIO1_DR = of_iomap(newchrled.node,3);
    IMX6U_GPIO1_GDIR = of_iomap(newchrled.node,4);

#endif





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



