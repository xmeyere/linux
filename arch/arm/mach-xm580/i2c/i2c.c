#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/io.h>

//NOTE: This file was orginally a kernel module (xm_i2c.ko), but I added it here for simplicity.
static DEFINE_MUTEX(g_i2c_lock);

int i2c_read(int dev_index, int device,int reg_addr,int addr_byte_num,int data_byte_num)
{
    int i2c_offset = dev_index * 0x1000;
    int i =0;
    mutex_lock(&g_i2c_lock);

    writel(device, (void*)(i2c_offset - 0x1fffff4));
    writel(addr_byte_num, (void*)(i2c_offset - 0x1ffffe8));

    unsigned int* weird_register = (unsigned int*)(i2c_offset - 0x2000000);
    unsigned int weird_register_org = *weird_register;

    *weird_register = weird_register_org | 0x40;
    *weird_register = weird_register_org & 0xffffffbf;

    if (addr_byte_num != 0)
    {
        i = 0;
        weird_register_org = (addr_byte_num + 0x1fffffff) * 8;
        while(i != addr_byte_num)
        {
            writel((int)reg_addr >> (weird_register_org & 0xff) & 0xff,(void*)(i2c_offset - 0x1fffff0));
            i++;
            weird_register_org-=8;
        }
    }

    writel(0, (void*)(i2c_offset - 0x1fffff0));
    *weird_register = 0xb;
    // most of this seems to be a duplicate of i2c_write (even a printk mentions it). probably the compiler inlining stuff.
    if ((readl((void*)(i2c_offset - 0x1ffffb8)) & 0x10) == 0)
    {
        i = 0x1001;
        while(true)
        {
            if ((readl((void*)(i2c_offset - 0x1ffffb8)) & 0x10) == 0) break;
            i--;
            if(i == 0)
            {
                printk("i2c_write timeout %x", readl((void*)(i2c_offset - 0x1ffffb8)));
                common_failure:
                *weird_register = 1;
                writel(0xffff, (void*)(i2c_offset - 0x1ffffbc));
                return -1;
            }
        }
    }

    writel(0x10, (void*)(i2c_offset - 0x1ffffbc));

    // this is where i2c_read actually beings
    writel(data_byte_num, (void*)(i2c_offset - 0x1ffffe4));

    weird_register_org = *weird_register;
    *weird_register = weird_register_org | 0x80; //set it to read mode?
    *weird_register = weird_register_org & 0xffffff7f;
    *weird_register = 0x803;

    if ((readl((void*)(i2c_offset - 0x1ffffb8)) & 0x20) != 0)
    {
        on_ready:
        printk("i2c_read: bit is present\n");
        int result = 0;
        if(data_byte_num == 0)
        {
            result = 0;
        }
        else{
            result = 0;
            i = 0;
            while(i != data_byte_num)
            {
                result = readl((void*)(i2c_offset - 0x1ffffec)) | result << 8;
                i++;
            }

            writel(0x20, (void*)(i2c_offset - 0x1ffffbc));
            mutex_unlock(&g_i2c_lock);
            printk("i2c_read normal exit\n");
            return result;
        }
    }
    else
    {
        i = 0x1001;
        while (i != 0)
        {
            if ((readl((void*)(i2c_offset - 0x1ffffb8)) & 0x20) == 0) goto on_ready;
            i--;
            if(i == 0)
            {
                printk("i2c_read timeout %x", readl((void*)(i2c_offset - 0x1ffffb8)));
                goto common_failure;
            }
        }
    }

    mutex_unlock(&g_i2c_lock);
    return -1;
}

int i2c_write(int dev_index,int device,int reg_addr,int addr_byte_num,int data,int data_byte_num)
{
    int i = 0;
    unsigned int org_ivar2 = 0;
    int* v18;
    int* device_pointer = (void*)((dev_index << 12) - 0x2000000);
    mutex_lock(&g_i2c_lock);

    writel(device, device_pointer + 3);
    writel(addr_byte_num + data_byte_num, device_pointer + 6);

    org_ivar2 = *device_pointer;
    *device_pointer = org_ivar2 | 0x40;
    *device_pointer = org_ivar2 & 0xffffffbf;
    v18 = device_pointer + 4;
    if (addr_byte_num != 0)
    {
        i = 0;
        
        while(i != addr_byte_num)
        {
            writel((int)reg_addr >> (org_ivar2 & 0xff) & 0xff, v18);
            i++;
            org_ivar2-=8;
        }
    }


    if (data_byte_num != 0)
    {
        i = 0;
        org_ivar2 = (data_byte_num - 0x1fffffff) * 8;
        while(i != data_byte_num)
        {
            writel((int)data >> (org_ivar2 & 0xff) & 0xff, v18);
            i++;
            org_ivar2-=8;
        }
    }

    writel(0, v18);
    *device_pointer = 0xb;
    if ((device_pointer[18] & 0x10) == 0)
    {
        printk("i2c_write: bit not present\n");
        i = 0x1001;
        while(true)
        {
            if ((device_pointer[18] & 0x10) == 0) break;
            i--;
            if(i == 0)
            {
                printk("i2c_write: timeout %x", device_pointer[18]);
                *device_pointer = 1;
                device_pointer[17] = 0xFFFF;
                return -1;
            }
        }
    }

    device_pointer[17] = 16;
    printk("end of i2c_write()\n");

    mutex_unlock(&g_i2c_lock);
    return 0;
}

static int __init xmi2c_init_module(void)
{
    int i;
    for (i = 0xFE000038; i != 0xFE004038; i += 4096 )
    {
        writel(0xA8, (void*)(i - 24));
        writel(0xA8, (void*)(i - 20));
        writel(0xA8, (void*)(i - 16));
        writel(0xA8, (void*)(i - 12));
        writel(0xA8, (void*)(i - 8));
        writel(0x50, (void*)(i - 4));
        writel(0xFC, (void*)(i));
    }
    printk("xm_i2c init is ok! now doing init of temperture sensor\n");
    if(i2c_write(3, 0x48, 1, 1, 0x20, 2) != 0)
    {
        printk("failed to init sensor\n");
    }

    if(i2c_write(3, 234, 1, 1, 0x20, 2) != 0)
    {
        printk("Write to non existant device test OK\n");
    }

    printk("i2c_read returned %d of temp sensor\n", i2c_read(3, 0x48, 0, 1, 2));
    printk("i2c_read returned %d of non existant device\n", i2c_read(3, 0x48, 0, 1, 2));
    return 0;
}

static void __exit xmi2c_cleanup_module(void)
{

}

module_init(xmi2c_init_module);
module_exit(xmi2c_cleanup_module);

MODULE_DESCRIPTION("Kernel module to allow XM580 RTC device to work");
MODULE_AUTHOR("Unknown author");
MODULE_LICENSE("GPL");