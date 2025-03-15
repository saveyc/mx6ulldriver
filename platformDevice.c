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
#include <asm/mach/map.h>
#include <asm/uacess.h>
#include <asm/io.h>

#define  CCM_CCGQ1_BASE            (0x020C406C)
#define  SW_MUX_GPIO1_IO03_BASE    (0x020E0068)
#define  SW_PAD_GPIO1_IO03_BASE    (0x020E02F4)
#define  GPIO1_DR_BASE             (0x0209C000)
#define  GPIO1_GDIR_BASE           (0x0209C004)
#define  REGISTER_LEN               0x4

static void  led_release(struct device *dev)
{
    printk("led device release! \r\n");
}

static struct resource led_resources[] = {
    [0] = {
        .start = CCM_CCGQ1_BASE,
        .end = CCM_CCGQ1_BASE + REGISTER_LEN - 1,
        .flags = IORESOURCE_MEM,
    },
    [1] = {
        .start = SW_MUX_GPIO1_IO03_BASE,
        .end = SW_MUX_GPIO1_IO03_BASE + REGISTER_LEN - 1,
        .flags = IORESOURCE_MEM,
    },
    [2] = {
        .start = SW_PAD_GPIO1_IO03_BASE,
        .end = SW_PAD_GPIO1_IO03_BASE + REGISTER_LEN - 1,
        .flags = IORESOURCE_MEM,
    },
    [3] = {
        .start = GPIO1_DR_BASE,
        .end = GPIO1_DR_BASE + REGISTER_LEN - 1,
        .flags = IORESOURCE_MEM,
    },
    [4] = {
        .start = GPIO1_GDIR_BASE,
        .end = GPIO1_GDIR_BASE + REGISTER_LEN - 1,
        .flags = IORESOURCE_MEM,
    }
}

static struct platform_device ledDevice = {
    .name = "imx6u-led",
    .id = -1;
    .dev = {
        .release = led_release,
    },
    .num_resources = ARRAY_SIZE(led_resources),
    .resource = led_resources,
};


static int __init led_init(void)
{
    return platform_device_register(&ledDevice);
}

static void __exit led_exit(void)
{
    platform_device_unregister(&ledDevice);
}

module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");



