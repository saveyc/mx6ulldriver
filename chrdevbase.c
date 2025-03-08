#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ide.h>

#define CHRDEVBASE_MAJOR   200
#define CHRDEVNAME_NAME    "chrdevbase"

static char readbuf[100];
static char writebuf[100];
static char kerneldate[] = {"kerneldate"};

static int chrdevbase_open(struct inode *inode, struct file *file)
{
    // printk("chrdevbase_open\r\n");
    return 0;
}

static ssize_t chrdevbase_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    int retvalue = 0;

    memcpy(readbuf, kerneldate, sizeof(kerneldate));
    retvalue = copy_to_user(buf, readbuf, count);

    if(retvalue == 0)
    {
        printk("chrdevbase_read: %s\r\n", readbuf);
    }
    else{
        printk("chrdevbase_read: error\r\n");
    }

    return 0 ;

}

static ssize_t chrdevbase_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    int retvalue = 0;
    
    retvalue = copy_from_user(writebuf, buf, count);
    if(retvalue == 0)
    {
        printk("chrdevbase_recvied: %s\r\n", writebuf);
    }
    else{
        printk("chrdevbase_recvied: error\r\n");
    }

    return 0;
}

static int chrdevbase_release(struct inode *inode, struct file *file)
{
    // printk("chrdevbase_release\r\n");
    return 0;
}

static struct file_operations chrdevbase_fops = {
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .read = chrdevbase_read,
    .write = chrdevbase_write,
    .release = chrdevbase_release,
};

static int __init chrdevbase_init(void)
{
    int retvalue = 0;

    retvalue = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVNAME_NAME, &chrdevbase_fops);
    if(retvalue < 0)
    {
        printk("chrdevbase_init: register_chrdev failed\r\n");
    }   

    printk("chrdevbase_init: %s\r\n", CHRDEVNAME_NAME);

    return 0;
}

static void __exit chrdevbase_exit(void)
{
    unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVNAME_NAME);
    printk("chrdevbase_exit\r\n");
}

module_init(chrdevbase_init);
module_exit(chrdevbase_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("k");



