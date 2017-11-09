#include<signal.h>
#include<stdio.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<stdlib.h>

void child(int sno)
{
	int stat,cno;
	cno=wait(&stat);
	printf("Child %d died or blocked\n",cno);
	printf("Exit code is %d\n",WEXITSTATUS(stat));
	//signal(SIGCHLD,child);
}

int main() {
	double t;

	srand(time(NULL));
	if (fork() && fork() && fork()) {
		signal(SIGCHLD,child);
		for (;;) sleep(1);
	} else {
		srand(getpid()*time(NULL));
		t=rand()*10.0/RAND_MAX;
		sleep((int) t);
		return(rand()%20);
	}
	return 0;
}
