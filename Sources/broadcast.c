#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<time.h>

#define SSIZE 100000

pthread_mutex_t mut=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t con=PTHREAD_COND_INITIALIZER;

int arr[SSIZE];

void * selsort(void * n) 
{
	struct timespec tm1,tm2;
	int i,j,min;
	int *p;

	p=malloc(SSIZE*sizeof(int));
	for (i=0; i<SSIZE; i++)
		p[i] = arr[i];

	pthread_mutex_lock(&mut);
	pthread_cond_wait(&con, &mut);
	pthread_mutex_unlock(&mut);
	clock_gettime(CLOCK_REALTIME,&tm1);
	printf("selection sort started %d:%d\n",tm1.tv_sec,tm1.tv_nsec);
	for (i=0; i<SSIZE; i++) {
		min=i;
		for (j=i+1; j<SSIZE; j++) 
			if (p[min] > p[j])
				min = j ; 
		if (min != i) {
			int tmp=p[i];
			p[i]=p[min];
			p[min]=tmp;
		}
	}
	
	clock_gettime(CLOCK_REALTIME,&tm2);
	printf("selection sort: %.6f\n", (tm2.tv_sec-tm1.tv_sec)+(tm2.tv_nsec-tm1.tv_nsec)/1e9);
	free (p);
	return NULL;
}

void * inssort(void * n) 
{
	struct timespec tm1,tm2;
	int i,j,k;
	int *p;

	p=malloc(SSIZE*sizeof(int));
	for (i=0; i<SSIZE; i++)
		p[i] = arr[i];

	pthread_mutex_lock(&mut);
	pthread_cond_wait(&con, &mut);
	pthread_mutex_unlock(&mut);
	clock_gettime(CLOCK_REALTIME,&tm1);
	printf("insertion sort started at %d:%d\n",tm1.tv_sec,tm1.tv_nsec);
	for (i=1; i<SSIZE; i++) {
		int tmp=p[i];

		for (j=i-1; j>=0 && p[j]>=tmp; j--)
			p[j+1] = p[j];
		p[j+1]=tmp;	
	}
	
	clock_gettime(CLOCK_REALTIME,&tm2);
	printf("insertion sort: %.6f\n", (tm2.tv_sec-tm1.tv_sec)+(tm2.tv_nsec-tm1.tv_nsec)/1e9);
	return NULL;
}

void * shlsort(void * n) 
{
	struct timespec tm1,tm2;
	int i,j,k;
	int *p;

	p=malloc(SSIZE*sizeof(int));
	for (i=0; i<SSIZE; i++)
		p[i] = arr[i];

	pthread_mutex_lock(&mut);
	pthread_cond_wait(&con, &mut);
	pthread_mutex_unlock(&mut);
	clock_gettime(CLOCK_REALTIME,&tm1);
	printf("shell sort started at %d:%d\n",tm1.tv_sec,tm1.tv_nsec);
	k = 1;
	while (k < SSIZE) {
		k = (3 * k) + 1;
	}

	while (k > 0) {
		for (i=k; i<SSIZE; i ++) {
			int tmp=p[i];

			for (j=i-k; j>=0 && p[j]>=tmp; j -= k)
				p[j+k] = p[j];

			p[j+k]=tmp;	
		}
		k /= 3;
	}
	
	clock_gettime(CLOCK_REALTIME,&tm2);
	printf("shell sort: %.6f\n", (tm2.tv_sec-tm1.tv_sec)+(tm2.tv_nsec-tm1.tv_nsec)/1e9);
	return NULL;
}

void * quicksort(void * n)
{
	struct timespec tm1,tm2;
	int i,j;
	int *p;
	int cmp(const void *a,const  void *b) { return   *(int *)a - *(int *)b; };

	p=malloc(SSIZE*sizeof(int));
	for (i=0; i<SSIZE; i++)
		p[i] = arr[i];

	pthread_mutex_lock(&mut);
	pthread_cond_wait(&con, &mut);
	pthread_mutex_unlock(&mut);
	clock_gettime(CLOCK_REALTIME,&tm1);
	printf("quick sort started %d:%d\n",tm1.tv_sec,tm1.tv_nsec);
	qsort(p, SSIZE, sizeof(int), cmp);
	clock_gettime(CLOCK_REALTIME,&tm2);
	printf("quick sort: %.6f\n", (tm2.tv_sec-tm1.tv_sec)+(tm2.tv_nsec-tm1.tv_nsec)/1e9);
	return NULL;

}

void * bubblesort(void * n) 
{
	struct timespec tm1,tm2;
	int i,j,swapped,k;
	int *p;

	p=malloc(SSIZE*sizeof(int));
	for (i=0; i<SSIZE; i++)
		p[i] = arr[i];

	pthread_mutex_lock(&mut);
	pthread_cond_wait(&con, &mut);
	pthread_mutex_unlock(&mut);
	clock_gettime(CLOCK_REALTIME,&tm1);
	printf("bubble sort started %d:%d\n",tm1.tv_sec,tm1.tv_nsec);

	swapped = 1;
	for (i=0; swapped && i<SSIZE-1; i++) {
		swapped = 0;
		for (j=0; j<SSIZE-i-1; j++)  {
			k = j+1;
			if (p[j] > p[k]) {
				int tmp = p[j];
				p[j] = p[k];
				p[k] = tmp;
				swapped = 1;
			}
		}
	}
	
	clock_gettime(CLOCK_REALTIME,&tm2);
	printf("bubble sort: %.6f\n", (tm2.tv_sec-tm1.tv_sec)+(tm2.tv_nsec-tm1.tv_nsec)/1e9);
	return NULL;
}

int main() {
	int i;
	pthread_t ssthr,isthr,bsthr,shthr,qsthr;
	srand(time(NULL));

	for  (i=0; i<SSIZE; i++)
		arr[i] = rand() % 1000000;

	pthread_create(&bsthr, NULL, bubblesort, NULL);
	pthread_create(&isthr, NULL, inssort, NULL);
	pthread_create(&ssthr, NULL, selsort, NULL);
	pthread_create(&shthr, NULL, shlsort, NULL);
	pthread_create(&qsthr, NULL, quicksort, NULL);

	sleep(1);
	pthread_cond_broadcast(&con);

	pthread_join(ssthr,NULL);
	pthread_join(isthr,NULL);
	pthread_join(bsthr,NULL);
	pthread_join(shthr,NULL);
	pthread_join(qsthr,NULL);

}
