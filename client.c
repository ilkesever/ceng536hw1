#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define SADDRESS     "./mysocket22" 

#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
	char c;
	FILE *fp;
	register int i, s, len;
	struct sockaddr_un saun;
	fd_set readset;
	int selres, nread;
	char line[BUF_SIZE];

	/*
	 * Get a socket to work with.  This socket will
	 * be in the UNIX domain, and will be a
	 * stream socket.
	 */
	if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
		perror("client: socket");
		exit(1);
	}

	/*
	 * Create the address we will be connecting to.
	 */
	saun.sun_family = AF_UNIX;
	strcpy(saun.sun_path, SADDRESS);

	/*
	 * Try to connect to the address.  For this to
	 * succeed, the server must already have bound
	 * this address, and must have issued a listen()
	 * request.
	 *
	 * The third argument indicates the "length" of
	 * the structure, not just the length of the
	 * socket name.
	 */
	len = sizeof(saun.sun_family) + strlen(saun.sun_path);

	if (connect(s, (struct sockaddr *)&saun, len) < 0) {
		perror("client: connect");
		exit(1);
	}

	while (1 ==1) {
		fgets(line,1024,stdin);
		nread=send(s,line,strlen(line)+1,0);

		if (nread<0)
			perror("writing:");
		else if(strcmp(line, "quit\n") == 0)
			break;
		else 			
			printf("wrote: %d bytes, %s\n",nread,line);

		nread=recv(s,line,BUF_SIZE,0);
    	printf("received: len=%d, content=%s\n",
		            	nread,line);
		if (nread<0)
			break;
	}


	/*
	 * We can simply use close() to terminate the
	 * connection, since we're done with both sides.
	 */
	close(s);

	exit(0);
}
