#include<stdio.h>
#include<stdlib.h>
#include<sys/ipc.h>
#include<sys/sem.h>

int semkeys[5]={0,0,0,0,0};
const struct sembuf  up = {0,1,SEM_UNDO};
const struct sembuf  down = {0,-1,SEM_UNDO};




int main(int argc,char *argv[]) 
{
	int i,n;
	for (i=1;i<argc;i++) {
		semctl(atoi(argv[i]),1,IPC_RMID);
	}
}

