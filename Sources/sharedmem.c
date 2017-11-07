#include<stdio.h>
#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/signal.h>
#include<sys/time.h>
#include<unistd.h>

void operate(int key)
{
	double (*array)[100];
	int i,j;
	srand(getpid());
	if ((array=shmat(key,0,0))==NULL) {
		perror("Attaching shared mem");
		exit(-1);
	}

	while (1) {
		for (i=0;i<100;i++) 
		for (j=0;i<100;i++) 
			array[i][j]=array[rand()%100][rand()%100];
		sleep(1);
	}

}

int main() {
	int sharedmem;

	if ((sharedmem=shmget(IPC_PRIVATE,sizeof(double)*10000,IPC_CREAT|0600))<0) {
			perror("Creating shared mem");
			exit(-1);
	}

	fork();
	operate(sharedmem);
	return 0;
}

	

