#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <time.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ADDRESS     "./mysocket"  /* addr to connect */

#define BUF_SIZE 1024


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
    int i,j, s, nread, len,plen;
    struct sockaddr_un saun;
    struct sockaddr_un paun;
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

    /*
     * Create the address we will be binding to.
     */
    saun.sun_family = AF_UNIX;
    strcpy(saun.sun_path, ADDRESS);

    /*
     * Try to bind the address to the socket.  We
     * unlink the name first so that the bind won't
     * fail.
     *
     * The third argument indicates the "length" of
     * the structure, not just the length of the
     * socket name.
     */
    unlink(ADDRESS);
    len = sizeof(struct sockaddr_un);

    if (bind(s, (struct sockaddr *)&saun, len) < 0) {
        perror("server: bind");
        exit(1);
    }

    for (;;) {
    	plen=len;
		nread = recvfrom(s,buf, BUF_SIZE, 0, 
			(struct sockaddr *) &paun, &plen);
		if (nread<0) {
			perror("receive");
			continue;
		}
		printf("peer address: len=%d, fam=%d, path=%s\n",
			plen,paun.sun_family, paun.sun_path);
		printf("peer request: size=%d, content=%s\n", nread, buf);
		servecommand(buf);
		sendto(s,buf,strlen(buf)+1,0,
			(struct sockaddr *) &paun,plen);
	}
	    
	    


    close(s);

    exit(0);
}
