#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/vmalloc.h>
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>
#include <linux/aio.h>
#include <linux/uio.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include "maplock536.c"
#include "fb536.h"


struct scullc_dev *fb536_devices;

int fb536_major =   SCULLC_MAJOR;
int width 		= DEFAULT_WIDTH;
int height 		= DEFAULT_HEIGTH;
int numminors 	= DEFAULT_MINORS;

module_param(numminors, int, 0);
module_param(width, int, 0);
module_param(height, int, 0);

MODULE_AUTHOR("Emir Kaan Perek");
MODULE_LICENSE("Dual BSD/GPL");




int fb536_open (struct inode *inode, struct file *filp)
{
	struct fb_area *window = kmalloc(sizeof(struct fb_area), GFP_USER);
	struct filedata *fdata = kmalloc(sizeof(struct filedata), GFP_USER);
	struct scullc_dev *dev = container_of(inode->i_cdev, struct scullc_dev, cdev);


	window->x = 0; 
	window->y = 0;
	window->width = dev->width;
	window->height = dev->height;
	
	fdata->active = 1;
	fdata->window = window;
	fdata->deviceptr = dev;
	list_add(&(fdata->list), &(dev->list));

	filp->private_data = fdata;

	//printk(KERN_INFO "The window is opening\n");	
	
	return 0;
}

int fb536_release (struct inode *inode, struct file *filp)
{

	struct filedata *fdata = filp->private_data;
	//struct scullc_dev *dev = fdata->deviceptr;

	list_del(&fdata->list);

	kfree(fdata->window);
	kfree(fdata);

	//printk(KERN_INFO "WINDOW IS CLOSING\n");
	
	return 0;
}

ssize_t fb536_read (struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{

	ssize_t retval = 0;
	int wrow, wcol, frow, fcol, i, lid;
	struct filedata *fdata = filp->private_data;
	struct scullc_dev *dev = fdata->deviceptr;

	if (!fdata->active)
		return -EBADF;

	lid = map_rdlock(fdata->window->x, fdata->window->y, fdata->window->x + fdata->window->width, fdata->window->y + fdata->window->height);
	if (lid == 0) return -ERESTARTSYS;

	//printk("read is called.. count: %d and offset: %d width: %d height: %d size: %d\n", 
	//	(int)count, (int)(*f_pos), (int)dev->width, (int)dev->height, (int)dev->size);
	
	
	if (*f_pos > (fdata->window->width * fdata->window->height))
		goto nothing;

	if ((*f_pos + count) > (fdata->window->width * fdata->window->height))
		count = (fdata->window->width * fdata->window->height) - *f_pos;

	wrow 	= ((long) *f_pos) / (fdata->window->width);
	wcol	= ((long) *f_pos) % (fdata->window->width);
	frow 	= fdata->window->y + wrow;
	fcol	= fdata->window->x + wcol;


	for (i=0; i<count; i++) {
		if (frow >= dev->height || fcol >= dev->width){retval = -EFAULT; goto nothing;}

		if (copy_to_user( buf+i, dev->ndata + (frow * dev->width + fcol), 1)){
			retval = -EFAULT;
			goto nothing;
		}
		wcol++;
		if (wcol == fdata->window->width){
			wcol = 0;
			wrow++;
		}

		frow = fdata->window->y + wrow;
		fcol = fdata->window->x + wcol;
	}

	*f_pos += count;
	map_unlock(lid);
	return count;

nothing:
	//printk("something wrong happened read \n");
	map_unlock(lid);
	return retval;
}


ssize_t fb536_write (struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;
	int wrow, wcol, frow, fcol, i, lid;
	char c;
	struct filedata *fdata = filp->private_data;
	struct scullc_dev *dev = fdata->deviceptr;

	if (!fdata->active)
		return -EBADF;

	lid = map_wrlock(fdata->window->x, fdata->window->y, fdata->window->x + fdata->window->width, fdata->window->y + fdata->window->height);
	if (lid == 0) return -ERESTARTSYS;

	//printk("write is called and count: %d and offset: %d op: %d\n", (int)count, (int)(*f_pos), (int)dev->op);

	if (*f_pos > (fdata->window->width * fdata->window->height))
		goto nothing;

	if ((*f_pos + count) > (fdata->window->width * fdata->window->height))
		count = (fdata->window->width * fdata->window->height) - *f_pos;

	wrow 	= ((long) *f_pos) / (fdata->window->width);
	wcol	= ((long) *f_pos) % (fdata->window->width);
	frow 	= fdata->window->y + wrow;
	fcol	= fdata->window->x + wcol;

	for (i=0; i<count; i++) {
		if (frow >= dev->height || fcol >= dev->width){retval = -EFAULT; goto nothing;}

		if (copy_from_user(&c, buf+i, 1)){
			retval = -EFAULT;
			goto nothing;
		}
		switch(dev->op) {
			case FB536_ADD:
				if (c + *((char*)(dev->ndata + (frow * dev->width + fcol))) > 255)
					*((char*)(dev->ndata + (frow * dev->width + fcol))) = 255;
				else
					*((char*)(dev->ndata + (frow * dev->width + fcol))) += c;
				break;
			case FB536_SUB:
				if (*((char*)(dev->ndata + (frow * dev->width + fcol))) - c < 0)
					*((char*)(dev->ndata + (frow * dev->width + fcol))) = 0;
				else
					*((char*)(dev->ndata + (frow * dev->width + fcol))) -= c;
				break;
			case FB536_AND:
				*((char*)(dev->ndata + (frow * dev->width + fcol))) &= c;
				break;
			case FB536_OR:
				*((char*)(dev->ndata + (frow * dev->width + fcol))) |= c;
				break;
			case FB536_XOR:
				*((char*)(dev->ndata + (frow * dev->width + fcol))) ^= c;
				break;
			default:
				*((char*)(dev->ndata + (frow * dev->width + fcol))) = c;
		}
		

		wcol++;
		if (wcol == fdata->window->width){
			wcol = 0;
			wrow++;
		}
		
		frow = fdata->window->y + wrow;
		fcol = fdata->window->x + wcol;
	}

	*f_pos += count;
	
	map_unlock(lid);
	return count;

nothing:
	//printk("something wrong happened write \n");
	map_unlock(lid);
	return retval;
}


void checkWindowsAndCrop(struct scullc_dev* dev, int width, int height){
	struct filedata *cursor;
	struct fb_area *w;
	list_for_each_entry(cursor, &(dev->list), list) {
		w = cursor->window;
		if (w->x < width && w->x + w->width > width)
			w->width = width - w->x;
		if (w->y < height && w->y + w->height > height)
			w->height = height - w->y;
		if (w->x > width || w->y > height)
			cursor->active = 0;
		/// ..... TO BE CONTINUED
	}
}

long fb536_ioctl (struct file *filp,
                 unsigned int cmd, unsigned long arg)
{

	int err = 0, ret = 0, tmp1, tmp2, lid;
	unsigned short x, y, w, h;
	unsigned long r;
	struct filedata *fdata;
	struct scullc_dev *dev;
	

	if (_IOC_TYPE(cmd) != FB536_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > FB536_IOC_MAXNR) return -ENOTTY;


	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if (err)
		return -EFAULT;


	fdata = filp->private_data;
	dev = fdata->deviceptr;

	if (!fdata->active)
		return -EBADF;

	switch(cmd) {

	case FB536_IOCRESET:
		lid = map_wrlock(0, 0, dev->width, dev->height);
		if (lid == 0) return -ERESTARTSYS;
		memset(dev->ndata, 0, dev->size);
		map_unlock(lid);
		break;

	case FB536_IOCTSETSIZE:
		tmp1 = arg >> 16;
		tmp2 = arg & 0xffff;
		if (tmp1 > 10000 || tmp1 < 1 || tmp2 > 10000 || tmp2 < 1)
			return -EINVAL;
		
		lid = map_wrlock(0, 0, dev->width, dev->height);
		if (lid == 0) return -ERESTARTSYS;

		vfree(dev->ndata);
		dev->width = tmp1;
		dev->height = tmp2;
		dev->size = tmp1 * tmp2;
		//printk("setsize with width %d height %d size %d\n", (int)dev->width, (int)dev->height, (int)dev->size);
		dev->ndata = (char*) vmalloc( dev->size * sizeof(char));
		
		memset(dev->ndata, 0, dev->size);

		checkWindowsAndCrop(dev, tmp1, tmp2);

		map_unlock(lid);
		
		break;

	case FB536_IOCQGETSIZE:

		r = dev->width << 16 | dev->height;
		return r;

	case FB536_IOCTSETOP:
		
		lid = map_wrlock(0, 0, dev->width, dev->height);
		if (lid == 0) return -ERESTARTSYS;

		//printk("SETOP is called old val: %d new val: %d \n", dev->op, (int)arg);
		dev->op = (int) arg;
		
		map_unlock(lid);
		

		break;

	case FB536_IOCQGETOP:
		return (long) dev->op;

	case FB536_IOCSETWINDOW:
		
		__get_user(x, (unsigned short __user *) &((struct fb_area __user *)arg)->x);
		__get_user(y, (unsigned short __user *) &((struct fb_area __user *)arg)->y);
		__get_user(w, (unsigned short __user *) &((struct fb_area __user *)arg)->width);
		__get_user(h, (unsigned short __user *) &((struct fb_area __user *)arg)->height);
		//printk("getusers of ioctl setwindow %d %d %d %d\n", x, y, w, h);

		if (x > dev->width || (x + w) > dev->width || y > dev->height || (y + h) > dev->height)
			return -EINVAL;
		fdata->window->x = x;
		fdata->window->y = y;
		fdata->window->width = w;
		fdata->window->height= h;
		
		break;

	case FB536_IOCGETWINDOW:
		__put_user(fdata->window->x, (unsigned short __user *) &((struct fb_area __user *)arg)->x);
		__put_user(fdata->window->y, (unsigned short __user *) &((struct fb_area __user *)arg)->y);
		__put_user(fdata->window->width, (unsigned short __user *) &((struct fb_area __user *)arg)->width);
		__put_user(fdata->window->height, (unsigned short __user *) &((struct fb_area __user *)arg)->height);
		
		break;


	default:
		return -ENOTTY;
	}

	return ret;
}


loff_t fb536_llseek (struct file *filp, loff_t off, int whence)
{
	struct filedata *fdata = filp->private_data;
	struct scullc_dev *dev = fdata->deviceptr;
	long newpos;

	if (!fdata->active)
		return -EBADF;

	switch(whence) {
	case 0: /* SEEK_SET */
		newpos = off;
		break;

	case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	case 2: /* SEEK_END */
		newpos = dev->size;
		break;

	default: /* can't happen */
		return -EINVAL;
	}
	if (newpos<0) return -EINVAL;
	filp->f_pos = newpos;

	return newpos;
}


 

struct file_operations fb536_fops = {
	.owner =     THIS_MODULE,
	.llseek =    fb536_llseek,
	.read =	     fb536_read,
	.write =     fb536_write,
	.unlocked_ioctl =     fb536_ioctl,
	.open =	     fb536_open,
	.release =   fb536_release,
	
};



static void fb536_setup_cdev(struct scullc_dev *dev, int index)
{
	int err, devno = MKDEV(fb536_major, index);
    
	cdev_init(&dev->cdev, &fb536_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &fb536_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	if (err)
		printk(KERN_NOTICE "Error %d adding scull%d", err, index);
}


int fb536_init(void)
{
	int result, i;
	dev_t dev = MKDEV(fb536_major, 0);
	
	
	if (fb536_major)
		result = register_chrdev_region(dev, numminors, DEVICE_NAME);
	else {
		result = alloc_chrdev_region(&dev, 0, numminors, DEVICE_NAME);
		fb536_major = MAJOR(dev);
	}
	if (result < 0)
		return result;

	
	fb536_devices = kmalloc(numminors * sizeof(struct scullc_dev), GFP_USER);
	if (!fb536_devices) {
		result = -ENOMEM;
		goto fail_malloc;
	}
	memset(fb536_devices, 0, numminors * sizeof(struct scullc_dev));
	for (i = 0; i < numminors; i++) {
		
		fb536_devices[i].op = FB536_SET;
		fb536_devices[i].width = width;
		fb536_devices[i].height = height;
		fb536_devices[i].size = width * height;
		fb536_devices[i].ndata = (char*) vmalloc(fb536_devices[i].size * sizeof(char));
		memset(fb536_devices[i].ndata, '0', fb536_devices[i].size);
		INIT_LIST_HEAD(&fb536_devices[i].list);
		mutex_init (&fb536_devices[i].mutex);
		fb536_setup_cdev(fb536_devices + i, i);
	}

	//printk( KERN_INFO "Driver successfully loaded with parameters minors: %d width: %d, height: %d\n", numminors, width, height);
	return 0; /* succeed */

  fail_malloc:
	unregister_chrdev_region(dev, numminors);
	return result;
}



void fb536_cleanup(void)
{
	int i;


	for (i = 0; i < numminors; i++) {
		cdev_del(&fb536_devices[i].cdev);
		vfree(fb536_devices[i].ndata);
	}

	kfree(fb536_devices);

	unregister_chrdev_region(MKDEV (fb536_major, 0), numminors);

	//printk(KERN_INFO "Driver successfully UNLOADED!!\n");
}


module_init(fb536_init);
module_exit(fb536_cleanup);
