#ifndef _MAPLOCK536_
#define _MAPLOCK536_

#define NR_MAPLOCK 	317
#define NR_MAPUNLOCK 	318

#define MAP_RDLOCK 	0
#define MAP_WRLOCK 	1
#define MAP_TRYLOCK 	2

#define map_rdlock(xlt, ylt, xrb, yrb) \
	syscall(NR_MAPLOCK, xlt, ylt, xrb, yrb, MAP_RDLOCK)

#define map_rdlock_try(xlt, ylt, xrb, yrb) \
	syscall(NR_MAPLOCK, xlt, ylt, xrb, yrb, MAP_RDLOCK | MAP_TRYLOCK)

#define map_wrlock(xlt, ylt, xrb, yrb) \
	syscall(NR_MAPLOCK, xlt, ylt, xrb, yrb, MAP_WRLOCK)

#define map_wrlock_try(xlt, ylt, xrb, yrb) \
	syscall(NR_MAPLOCK, xlt, ylt, xrb, yrb, MAP_WRLOCK | MAP_TRYLOCK)

#define map_unlock(id) \
	syscall(NR_MAPUNLOCK, id)

#endif
