#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <asm/io.h>

MODULE_AUTHOR("Ryuichi Ueda and Jitsukawa Hikaru");
MODULE_DESCRIPTION("driver control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.1");

#define REG(addr) (*((volatile unsigned int*)(addr)))
#define num 2
#define REG_ADDR_BASE (0xfe000000)
#define REG_ADDR_GPIO_BASE (REG_ADDR_BASE + 0x00200000)
#define REG_ADDR_GPIO_LEVEL_0 0x0034

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;

static int gpio[num] = {24,25};

static int mydriver_open(struct inode *inode, struct file *file){
	printk("mydriver_open");
	gpio_base[7] = 1 << gpio[0];
	return 0;
}

static int mydriver_close(struct inode *inode, struct file *file){
	printk("mydriver_close");
	gpio_base[10] = 1 << gpio[0];
	return 0;
}

static ssize_t led_write(struct file *filp, const char *buf, size_t count, loff_t *pos){
	char c;
	if(copy_from_user(&c, buf, sizeof(char)))
		return -EFAULT;

	if(c == '0'){
		gpio_base[7] = 1 << gpio[0];
	}
	else if(c == '1'){
		gpio_base[7] = 1 << gpio[1];
	}
	else if(c == '2'){
		gpio_base[10] = 1 << gpio[0];
		gpio_base[10] = 1 << gpio[1];
	}
	else if(c == '3'){
		gpio_base[7] = 1 << gpio[1];
		msleep(100);
		gpio_base[10] = 1 << gpio[1];
	}
	return 1;
}

static ssize_t test_read(struct file *filp, char __user *buf, size_t count, loff_t *pos){
	printk("led_read");
	buf[0]  ='A';

	return 1;
}

static struct file_operations driver_fops = {
	.owner   = THIS_MODULE,
	.open    = mydriver_open,
	.release = mydriver_close,
	.write   = led_write,
	.read    = test_read
};

static int __init init_mod(void){
	int i, retval;
	retval = alloc_chrdev_region(&dev, 0, 1, "mydriver");
	if(retval < 0){
		printk(KERN_ERR "alloc_chrdev_region failed.\n");
		return retval;
	}
	printk(KERN_INFO "%s is loaded. major:%d\n", __FILE__,MAJOR(dev));

	cdev_init(&cdv, &driver_fops);
	retval = cdev_add(&cdv, dev, 1);
	if(retval < 0){
		printk(KERN_ERR "cdev_add failed. major:%d, minor:%d", MAJOR(dev), MINOR(dev));
		return retval;
	}

	cls = class_create(THIS_MODULE, "mydriver");
	if(IS_ERR(cls)){
		printk(KERN_ERR "class_create failed.");
		return PTR_ERR(cls);
	}

	device_create(cls, NULL, dev, NULL, "mydriver%d", MINOR(dev));

	gpio_base = ioremap_nocache(REG_ADDR_GPIO_BASE, 0xa0);

	for(i = 0; i < num; i++){
		const u32 led = gpio[i];
		const u32 index = led/10;
		const u32 shift = (led%10)*3;
		const u32 mask = ~(0x7 << shift);
		gpio_base[index] = (gpio_base[index] & mask) | (0x1 << shift);
	}
	return 0;
}

static void __exit cleanup_mod(void){
	cdev_del(&cdv);
	device_destroy(cls, dev);
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO "%s is unloaded. major:%d\n", __FILE__, MAJOR(dev));
}

module_init(init_mod);
module_exit(cleanup_mod);

