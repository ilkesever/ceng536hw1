#include <linux/syscalls.h>
#include <linux/printk.h>
#include <linux/mutex.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include "maplock536.h"

struct lock {
	int id;
	int type;
	unsigned long x;
	unsigned long y;
	unsigned long w;
	unsigned long h;
	struct list_head list_arg;
	struct completion job;
};

LIST_HEAD(head);
DEFINE_MUTEX(mymutex);

int doesLocksIntersect(struct lock *l1, struct lock *l2){
	unsigned long xmin, xmax, ymin, ymax, rxmin, rxmax, rymin, rymax;
	xmin = l1->x; xmax = /*l1->x +*/ l1->w;
	ymin = l1->y; ymax = /*l1->y + */l1->h;
	rxmin = l2->x; rxmax = /*l2->x +*/ l2->w;
	rymin = l2->y; rymax = /*l2->y +*/ l2->h;
	if (
	(	 (rxmin>=xmin && rxmin<=xmax) ||
		(rxmin<=xmin && rxmax>=xmax) ||
		(rxmax>=xmin && rxmax<=xmax)
	)
	&&
	(
		(rymin>=ymin && rymin<=ymax) ||
		(rymin<=ymin && rymax>=ymax) ||
		(rymax>=ymin && rymax<=ymax)
	)
	&& (l1->type == 1 || l2->type == 1)
	)
		return 1;
	else return 0;
}

int firstAvailableID(void) {
	short doesExist;
	int avail_id = 1;
	struct lock* tmp;

	while(1) {
		doesExist = 0;
		list_for_each_entry(tmp, &head, list_arg) {
			if (tmp->id == avail_id){
				doesExist = 1;
				break;
			}
		}
		if (doesExist == 1)
			avail_id++;
		else
			return avail_id;
	}
}

SYSCALL_DEFINE5(map_lock , unsigned long, xlt , unsigned long, ylt ,
		unsigned long , xrb , unsigned long, yrb , short, flags) {
	struct lock* newlock;
	struct lock* tmp;
	short isIntersected;
	int lock_id, waitreturn;

//	down(&mutex);
	mutex_lock(&mymutex);
	lock_id = 0;

	newlock = (struct lock*) kmalloc(sizeof(struct lock), GFP_KERNEL);
	newlock->x = xlt;
	newlock->y = ylt;
	newlock->w = xrb;
	newlock->h = yrb;
	newlock->type = flags & MAP_WRLOCK;
	newlock->id = lock_id;

	init_completion(&newlock->job);
//	printk("start for checking! \n");
	do {
		isIntersected = 0;
		list_for_each_entry(tmp, &head, list_arg) {
//			printk("check for id: %d\n", tmp->id);
			if ( doesLocksIntersect(tmp, newlock) == 1 ) {
				if ((flags & MAP_TRYLOCK) > 0){
					kfree(newlock);
//					printk("try lock is failed cuz intersect\n");
					//up(&mutex);
					mutex_unlock(&mymutex);
					return 0;
				}

//				printk("%d intersect with id %d, wait\n",lock_id, tmp->id);
				isIntersected = 1;
				//up(&mutex);
				mutex_unlock(&mymutex);
				waitreturn = wait_for_completion_interruptible(&tmp->job);
				if (waitreturn != 0){
//					printk("wait is interrupted so return\n ");
					return 0;
				}
				//down(&mutex);
				mutex_lock(&mymutex);
//				printk("my id = %d waiting is over for id %d\n",lock_id, tmp->id);
				break;
			}
		}
	} while (isIntersected == 1);

	lock_id = firstAvailableID();
	newlock->id = lock_id;
	list_add(&newlock->list_arg, &head);

	//up(&mutex);
	mutex_unlock(&mymutex);

	return lock_id;
}

SYSCALL_DEFINE1(map_unlock , int, lockid ) {
	struct lock* tmp, *safetmp;
	int retvalue = 0, listsize=0;

	//down(&mutex);
	mutex_lock(&mymutex);
//	printk("start unlock id: %d\n", lockid);
	list_for_each_entry_safe(tmp, safetmp, &head, list_arg){
		if (tmp->id == lockid){
			list_del(&tmp->list_arg);
//			printk("deleting from list is done\n");
			retvalue = 1;
			complete_all(&tmp->job);
//			printk("completeall is done\n");
			kfree(tmp);
//			printk("kfree is done\n");
		}
		listsize++;
	}
	//printk("after deletion:::\n");
	//list_for_each_entry(tmp, &head, list_arg){
	//	printk("id: %d\n", tmp->id);
	//}
//	printk("end unlock size of list: %d, id : %d\n", listsize, lockid);
	//up(&mutex);
	mutex_unlock(&mymutex);

	return retvalue;
}
