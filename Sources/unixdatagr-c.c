#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define SADDRESS     "./mysocket2"  	/* addr to connect */
#define CADDRESSH     "./clientsocket-%d"   /* addr to get result */
char caddress[1024];

#define BUF_SIZE 1024

void cleanup() {
	unlink(caddress);
	exit(0);
}

int main()
{
    char c;
    FILE *fp;
    int fromlen;
    int i,j, s, nread, len,clen;
    time_t now;
    struct sockaddr_un saun;
    struct sockaddr_un caun;
    char buf[BUF_SIZE];

    /*
     * Get a socket to work with.  This socket will
     * be in the UNIX domain, and will be a
     * stream socket.
     */
    if ((s = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("server: socket");
        exit(1);
    }

    saun.sun_family = AF_UNIX;
    strcpy(saun.sun_path, SADDRESS);
    len = sizeof(struct sockaddr_un);

    caun.sun_family = AF_UNIX;
    sprintf(caddress,CADDRESSH,getpid());
    strcpy(caun.sun_path, caddress);
    clen = sizeof(struct sockaddr_un);
    unlink(caddress);
    if (bind(s, (struct sockaddr *)&caun, clen) < 0) {
        perror("client: bind");
        exit(1);
    }
	signal(SIGINT,cleanup);
	signal(SIGTERM,cleanup);

    for (;;) {
    		printf("command?:");
    		fgets(buf,BUF_SIZE,stdin);
		nread = sendto(s,buf, strlen(buf)+1, 0, 
			(struct sockaddr *) &saun, len);
		printf("wrote: %d\n", nread);
		nread = recv(s,buf, BUF_SIZE, 0 );
		if (nread<0 )
			perror("client: recv");
		else
			printf("received: %d bytes,\n%s\n", nread, buf);
	}
	    
	    


    close(s);
    unlink(caddress);
    exit(0);
}
