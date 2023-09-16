#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/time.h>
#include <linux/kfifo.h>
#include <linux/slab.h>
#include <mach/hardware.h>

// NOTE: This file was orginally a kernel module (xm_i2s.ko), but I added it
// here for simplicity.
void* reg_hdmi_base_va;
struct audio_channel {
  struct __kfifo fifo;
  wait_queue_head_t wq;
  raw_spinlock_t lock;
};

static struct audio_channel channels[11];
static struct miscdevice devices[11];
static int open_counts[11];
static int gpio = 0;

static long ao_ioctl(struct file *file,uint cmd_in,unsigned long arg){return 0;}
static ssize_t ao_write(struct file * file, const char * buffer, size_t buffer_size, loff_t * off);
static int ao_open(struct inode * node, struct file * file);
static int ao_release(struct inode * node, struct file * file);
static const struct file_operations ao_fops = {
    .write = ao_write,
    .unlocked_ioctl = ao_ioctl,
    .open = ao_open,
    .release = ao_release,
};

static const struct file_operations ai_fops = {
   
};

static irqreturn_t i2s0_interrupt(int irq, void *data) {
 printk("!!!!audio int 0\n");
 panic("the interupt s0!");
  return IRQ_HANDLED;
}

static irqreturn_t i2s1_interrupt(int irq, void *data) {
  printk("!!!audio int 1\n");
   panic("the interupt s1!");
  return IRQ_HANDLED;
}

int SysRegWrite(unsigned long address,unsigned long value)
{
  void* ptr = ioremap(address, 8);
  *(unsigned long *)((ptr)) = value;
  iounmap(ptr);
  return 0;
}

static int __init xmi2s_init_module(void) {
  char device_names[11][5] = {"ai0", "ai1", "ai2", "ai3", "ai4", "ai5",
                              "ai6", "ai7", "ai",  "ao",  "hdmi"};
  int i =0;
  printk("initializing audio output!\n");

  //pll.sh
  SysRegWrite(0x10020324,0x88);//ethernet led

  //This code is important as it powers on HDMI and other system stuff. dont know how it works.
  SysRegWrite( 0x20000000 ,0x1);//
  SysRegWrite( 0x20000040, 0x01034B4B);//	 #videoclk0  video bus  	mipicsi(1782/12=148.5)
  SysRegWrite( 0x20000044 ,0x7F7F5757);//   #videoclk1  lcd  ahd dis 	hdmi(1782/12=148.5)
  SysRegWrite( 0x20000048 ,0xFFFFFF04);//   #videoclk2  bpu vpss
  SysRegWrite( 0x2000004C, 0x0000007F);//	 #videoclk3  merge2x
  SysRegWrite( 0x20000060 ,0x00FFFFFF);//  #dspclk
  SysRegWrite( 0x2000006C, 0x410440FF);//  #veduclk0    enc edc
  SysRegWrite( 0x20000070, 0x40030000);//  #veduclk1    vedu bus
  SysRegWrite( 0x20000074, 0x00004104);//  #veduclk2    vedu apb clk
  SysRegWrite( 0x20000010, 0x12044A44);//  #pllb 1782M
  SysRegWrite( 0x20000000,0x0);//
  SysRegWrite( 0x2000011C, 1);

  SysRegWrite( 0x100B0040, 0x23);
  SysRegWrite( 0x100B0060, 0x6);
  //begin audio driver

  writel(0x84, (void *)(0xfe020028));
  writel(0x84, (void *)(0xfe02002c));
  writel(0x84, (void *)(0xfe020030));
  writel(0x88, (void *)(0xfe020048));
  writel(0x88, (void *)(0xfe02004c));

  // todo: GPIO if statement
  writel(0x30, (void *)(0xfe065000));

  writel(0x0, (void *)(0xfe065010));
  writel(0x0, (void *)(0xfe065020));

  writel(0x61f, (void *)(0xfe065004));
  writel(0xf, (void *)(0xfe065008));
  writel(0x10, (void *)(0xfe06500c));
  writel(0x3c0, (void *)(0xfe065018));
  writel(0x3c0, (void *)(0xfe06502c));
  writel(0xff001f, (void *)(0xfe065030));
  writel(0x200b0, (void *)(0xfe065000));
  writel(0x30, (void *)(0xfe066000));

  writel(0, (void *)(0xfe066010));
  writel(0, (void *)(0xfe066020));
  writel(0x61f, (void *)(0xfe066004));
  writel(0xf, (void *)(0xfe066008));
  writel(6, (void *)(0xfe06600c));
  writel(0x1e0, (void *)(0xfe066018));
  writel(0x1e0, (void *)(0xfe06602c));
  writel(0x283f8, (void *)(0xfe066000));
  writel(0x30, (void *)(0xfe067000));

  writel(0, (void *)(0xfe067010));
  writel(0, (void *)(0xfe067020));
  writel(0x61f, (void *)(0xfe067004));
  writel(0xf, (void *)(0xfe067008));
  writel(0x8, (void *)(0xfe06700c));
  writel(0xa0, (void *)(0xfe067018));
  writel(0xa0, (void *)(0xfe06702c));
  writel(0x2c2b8, (void *)(0xfe067000));
  for(i = 0; i < 11; i++)
  {
    init_waitqueue_head(&channels[i].wq);
    if(__kfifo_alloc(&channels[i].fifo, 3200, 1, 208) != 0)
    {
      printk("failed to allocate audio buffer!\n");
      return -1;
    }
  }

  if (request_threaded_irq(0x2f, i2s0_interrupt, NULL, 0x20, "audio", NULL) == 0 &&
      request_threaded_irq(0x30, i2s1_interrupt, NULL, 0x20, "audio", NULL) == 0)
  {
    reg_hdmi_base_va = ioremap(0x31045000,0x1000);
    if (reg_hdmi_base_va == 0)
    {
      printk("audio: hdmi ioremap in base failed\n");
      return -1;
    }
    printk("mapped hdmi\n");
    *(int *)(reg_hdmi_base_va + 0xe0) = 0;
    printk("initialized hdmi audio\n");
    if (request_threaded_irq(0x31, i2s1_interrupt, NULL, 0x20, "audio", NULL) == 0)
    {
       struct miscdevice* dev = devices;
      for(i = 0; i < 11; i++)
      {
        char* name = device_names[i];
        dev->minor = i + 100;
        dev->fops = i < 9 ? &ai_fops : &ao_fops;
        dev->name = name;
        printk("register device: %s\n", name);
        if(misc_register(dev) != 0)
        {
          printk("failed to register device: %s\n", name);
          return -19;
        }
        dev++;
      }
    }
    else
    {
       printk("failed to register HDMI audio interupts\n");
    }
  }
  else {
    printk("failed to register audio interupts\n");
  }

  printk("audio init OK\n");
  return 0;
}

static void __exit xmi2s_cleanup_module(void)
{
struct miscdevice* dev = devices;
int i=0;
while(true)
{
  misc_deregister(dev);
  dev++;
  i++;
  if(i>=11){break;}
}
  free_irq(0x2f, NULL);
  free_irq(0x30, NULL);
  free_irq(0x31, NULL);

  //todo: free fifo

  iounmap(reg_hdmi_base_va);
}

static ssize_t ao_write(struct file * file, const char * buffer, size_t buffer_size, loff_t * off)
{
  int channel = iminor(file->f_inode) - 100;
  uint spinner;
  void* kernel_buffer;
  //printk("ao_write todo with channel %d\n", channel);
  if((file->f_flags & 0x800) == 0)
  {
   // printk("0x800 flag not set\n");
    if((channels[channel].fifo.mask + 1) - (channels[channel].fifo.in - channels[channel].fifo.out) < buffer_size)
    {
        //TODO
            //printk("!stuff not implemented!\n");
    }
  }
  else if (channels[channel].fifo.mask < channels[channel].fifo.in - channels[channel].fifo.out)
  {
    return -11;
  }

  kernel_buffer = (void*)kmalloc(buffer_size, GFP_KERNEL);
  if(!kernel_buffer)
  {
    printk("failed to allocate temp buffer for audio buffer\n");
    return -1;
  }
  if(copy_from_user(kernel_buffer, buffer, buffer_size) != 0)
  {
    kfree(kernel_buffer);
    printk("audio: ao_hdmi_write copy_from_user error.\n");
    return -1;
  }

  spinner = _raw_spin_lock_irqsave(&channels[channel].lock);
  __kfifo_in(&channels[channel].fifo, kernel_buffer, buffer_size);
  _raw_spin_unlock_irqrestore(&channels[channel].lock, spinner);

  //todo
  kfree(kernel_buffer);
  return 0;
}
static int ao_open(struct inode * node, struct file * file)
{
  int channel = iminor(file->f_inode) - 100;
  printk("ao_open\n");

  if(open_counts[channel] == 0)
  {
    open_counts[channel]++;

    //TODO: add gpio module arg. Not sure if it's used on other NVR's, but on mine it isn't. It defaults to 0.
    if(gpio < 1 || channel != 9)
    {
      if(channel == 10)
      {
        writel(0x3c, (void*)(reg_hdmi_base_va + 0xe0)); //enable the interupt?
        writel(4, (void*)(reg_hdmi_base_va + 0x14));
        writel(0, (void*)(reg_hdmi_base_va + 0x14));
      }
    }
    else
    {
      //todo: write to some GPIO
      int gpioptr = 4 * (gpio + 0x4008000);
      int* ptr2;
      if ( gpioptr <= 0x1FFFFFFF )
        ptr2 = (int *)(gpioptr - 0x12000000);
      else
        ptr2 = (int *)(gpioptr - 0x22000000);
      if(!(gpioptr <= 0x1FFFFFFF))
      {
        ptr2 += 0x40000;
      }
      *ptr2=0xc00;
    }

    channels[channel].fifo.out = 0;
    channels[channel].fifo.in = 0;
    return 0;
  }
  else
  {
    //note: the standard driver does not return -1, it always returns 0 if it is open.
    printk("audio: cannot open channel %d as it is already opened!\n", channel);
    return -1;
  }

  return -1;
}
static int ao_release(struct inode * node, struct file * file)
{
  int channel = iminor(file->f_inode) - 100;
  printk("ao_release\n");
  open_counts[channel]--;
  return 0;
}

module_init(xmi2s_init_module);
module_exit(xmi2s_cleanup_module);

MODULE_DESCRIPTION("Kernel module to allow XM580 audio device to work");
MODULE_AUTHOR("Unknown author");
MODULE_LICENSE("GPL");