#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/rtc.h>
#include <linux/io.h>
#include <linux/time.h>
#include <mach/hardware.h>

//NOTE: This file was orginally a kernel module (acodec).ko), but I added it here for simplicity.
static long acodec_ioctl(struct file *file,uint cmd_in,unsigned long arg);
static const struct file_operations acodec_fops = {
    .unlocked_ioctl = acodec_ioctl
};

static struct miscdevice xm_acodec_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "acodec",
    .fops = &acodec_fops,
};

static int __init xmacodec_init_module(void)
{
  int var2 = 0x81;
  int i = 0;
  printk("initilizing audio codec!\n");
  writel(0, (void*)(0xfe068000));
  writel(3, (void*)(0xfe068000));
  writel(0x40, (void*)(0xfe06809c));

  while(i != 0x100)
  {
    i = var2 + 1;
    writel(var2, (void*)(0xfe0680a0));
    var2 = i;
  }
  var2 = 0x15;
  //Couldn't they use sleep() or something??
  while(true)
  {
    var2 = var2 - 1;
    if (var2 == 0) break;
  }
  writel(0x81, (void*)(0xfe0680a0));
  writel(0x10, (void*)(0xfe068008));
  writel(2, (void*)(0xfe06800c));
  writel(0x10, (void*)(0xfe068010));
  writel(2, (void*)(0xfe068014));
  writel(0xf8, (void*)(0xfe068084));
  writel(0x1f, (void*)(0xfe068088));
  writel(0x3f, (void*)(0xfe06808c));
  writel(0xd0, (void*)(0xfe068090));
  writel(0x2f, (void*)(0xfe068094));
  writel(0xc0, (void*)(0xfe068098));
  writel(0x99, (void*)(0xfe06809c));

  if (misc_register(&xm_acodec_dev) != 0) {
    printk("acodec failed to register device\n");
    return -1;
  }
  return 0;
}

static void __exit xmacodec_cleanup_module(void)
{
    //turn off the device
    writel(0, (void*)(0xfe068000));
    misc_deregister(&xm_acodec_dev);
}

static long acodec_ioctl(struct file *file,uint cmd_in,unsigned long arg)
{
    printk("acodec: nothing is implemented!");
    return -1;
}

module_init(xmacodec_init_module);
module_exit(xmacodec_cleanup_module);

MODULE_DESCRIPTION("Kernel module to allow XM580 audio codec device to work");
MODULE_AUTHOR("Unknown author");
MODULE_LICENSE("GPL");