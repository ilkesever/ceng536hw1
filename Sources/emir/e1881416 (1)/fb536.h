/* -*- C -*-
 * scullc.h -- definitions for the scullc char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 */

#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/list.h>

#define SCULLC_MAJOR 0

#define DEFAULT_WIDTH 1000
#define DEFAULT_HEIGTH 1000
#define DEFAULT_MINORS 4


#define DEVICE_NAME "fb536"

struct fb_area {
	unsigned short x, y;
	unsigned short width, height;
};

struct scullc_dev {
	char *ndata;
	int op;
	size_t size;
	unsigned short width;
	unsigned short height;
	struct list_head list;
	struct mutex mutex;
	struct cdev cdev;
};

struct filedata {
	struct list_head list;
	struct fb_area *window;
	struct scullc_dev *deviceptr;
	int active;
};

#define FB536_SET       0
#define FB536_ADD       1
#define FB536_SUB       2
#define FB536_AND       3
#define FB536_OR        4
#define FB536_XOR       5

#define FB536_IOC_MAGIC  'F'

#define FB536_IOCRESET 		_IO(FB536_IOC_MAGIC, 0)
#define FB536_IOCTSETSIZE 	_IO(FB536_IOC_MAGIC, 1)
#define FB536_IOCQGETSIZE 	_IO(FB536_IOC_MAGIC, 2)
#define FB536_IOCSETWINDOW 	_IOW(FB536_IOC_MAGIC, 3, struct fb_area)
#define FB536_IOCGETWINDOW 	_IOR(FB536_IOC_MAGIC, 4, struct fb_area)
#define FB536_IOCTSETOP 	_IO(FB536_IOC_MAGIC, 5)
#define FB536_IOCQGETOP 	_IO(FB536_IOC_MAGIC, 6)

#define FB536_IOC_MAXNR 6