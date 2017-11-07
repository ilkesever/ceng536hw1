#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define ADDRESS     "./mysocket3"  /* addr to connect */

#define BUF_SIZE 1024

void childwait(int p) {
	int stat;
	wait(&stat);
	signal(SIGCHLD,childwait);
}


void servecommand(char *buf) {
	time_t now;
	if (strncmp(buf,"TIME",4)==0) {
	    now=time(NULL);
		strncpy(buf,ctime(&now),BUF_SIZE);
	} else if (strncmp(buf,"HOSTNAME",8)==0) {
		gethostname(buf,BUF_SIZE);
	} else {
		strcpy(buf,"Usage: TIME\n");
	}
}

int main()
{
    char c;
    FILE *fp;
    int fromlen;
    int i,j, s, nread, len,plen,ns;
    struct sockaddr_un saun;
    struct sockaddr_un paun;
    char buf[BUF_SIZE];

    /*
     * Get a socket to work with.  This socket will
     * be in the UNIX domain, and will be a
     * stream socket.
     */
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("server: socket");
        exit(1);
    }

    /*
     * Create the address we will be binding to.
     */
    saun.sun_family = AF_UNIX;
    strcpy(saun.sun_path, ADDRESS);

    /*unlink(ADDRESS);*/
    plen=len = sizeof(struct sockaddr_un);

    if (bind(s, (struct sockaddr *)&saun, len) < 0) {
        perror("server: bind");
        exit(1);
    }

    ns=listen(s,10);
    if (ns<0) {
    	perror("server: listen");
		exit(1);
    }

	signal(SIGCHLD,childwait);

    while ((ns=accept(s,(struct sockaddr *)&paun,&plen))>=0) {
	    printf("accepted peer address: len=%d, fam=%d, path=%s\n",
			            plen,paun.sun_family, paun.sun_path);

		if (fork())
			continue;
		/* Following is the service process code 
		 * with connection established           */
		while (1) {
			nread=recv(ns,buf,BUF_SIZE,0);
	    	printf("received: len=%d, content=%s\n",
			            	nread,buf);
			if (nread<0)
				return(1);
			servecommand(buf);
			nread=send(ns,buf,strlen(buf)+1,0);
			if (nread<0)
				perror("writing:");
			else 			
				printf("wrote: %d bytes, %s\n",nread,buf);
		}
		return 0;
	}

    close(s);

    exit(0);
}
