#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define SADDRESS     "./mysocket3" 

int main(int argc, char *argv[])
{
	char c;
	FILE *fp;
	register int i, s, len;
	struct sockaddr_un saun;
	fd_set readset;
	int selres;
	char line[1024];

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

	/*fcntl(s,F_SETFL,O_NONBLOCK);*/
	FD_ZERO(&readset);
		FD_SET(0,&readset);
		FD_SET(s,&readset);
	while (selres=select(s+1,&readset,NULL,NULL,NULL)) {
		if (selres<0) {
			perror("select");
			return -2;
		}
		if (FD_ISSET(s,&readset)) {
			len=read(s,line,1024);
			if (len==0) return -1;
			write(1,line,len);
		}
		if (FD_ISSET(0,&readset)) {
			fgets(line,1024,stdin);
			write(s,line,strnlen(line,1024));
		}
		FD_ZERO(&readset);
		FD_SET(0,&readset);
		FD_SET(s,&readset);
		/*printf("selecting\n");*/
	}
	/*
	 * We can simply use close() to terminate the
	 * connection, since we're done with both sides.
	 */
	close(s);

	exit(0);
}
