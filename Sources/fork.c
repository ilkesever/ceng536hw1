#include<stdio.h>
#include<sys/types.h>
#include<sys/wait.h>
#include <unistd.h>

int main() {
	int c1,c2;
	int a=1;
	int c1ok,c2ok;
	int status,ret;

	c1ok=0; c2ok=0;
	if ((c1=fork()) && (c2=fork())) {
		printf("P| Children are %d and %d\n",c1,c2);
		printf("P| a is %d\n",a);
		while (! (c1ok && c2ok)) {
			ret=waitpid(0,&status,WNOHANG);
			if (ret == c1) 
				c1ok=1;
			else if (ret ==c2)
				c2ok=1;
			//if (ret)
				printf("P| %d returns %d\n",ret,WEXITSTATUS(status));
		}
	} else {
		printf("C| I'am %d :",getpid());
		a+=getpid();
		printf(" a is %d\n",a);
		return a%10;
	}
}
