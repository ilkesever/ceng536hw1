#include<signal.h>
#include<stdio.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<stdlib.h>

void child(int sno)
{
	printf("sno = %d\n",sno);
	printf("SIGCHLD = %d\n",SIGCHLD);
	int stat,cno;
	//cno=wait(&stat);
	printf("Child %d died or blocked\n",cno);
	printf("Exit code is %d\n",WEXITSTATUS(stat));
	printf("SIGCHLD = %d\n",SIGCHLD);
	//signal(SIGCHLD,child);
}

int main() {
	double t;
	int ppid = getpid();
	srand(time(NULL));
	if (fork() && fork() && fork()) {
		signal(SIGINT,child);
		for (;;) sleep(1);
	} else {
		printf("pid = %d\n",getpid());
		srand(getpid()*time(NULL));
		t=rand()*10.0/RAND_MAX;
		sleep((int) t);
		kill(ppid, SIGINT);
		return(123);
	}
	return 0;
}
