#include<stdio.h>
#include<sys/fcntl.h>
#include<sys/mman.h>

struct data {
	int x, y;
	double xstat, ystat;
};


int main() {
	int fd,i;
	struct data *arr;
	double sumx,sumy;

	fd=open("data.bin",O_RDWR);

	arr=(struct data *)mmap(0, 1000*sizeof(struct data), 
		PROT_READ|PROT_WRITE,
		MAP_SHARED, fd, 0);

	sumx=sumy=0;
	for (i=0;i<1000;i++) {
		sumx += arr[i].x;
		sumy += arr[i].y;
	}
	for (i=0;i<1000;i++) {
		arr[i].xstat = arr[i].x/sumx;
		arr[i].ystat = arr[i].y/sumy;
	}
	munmap(arr,1000);
	close(fd);
	return 0;
}




