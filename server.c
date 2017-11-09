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
#include<sys/ipc.h>
#include<sys/shm.h>
#include <stdbool.h>

#define BUF_SIZE 1024

void childwait(int p) {
	int stat;
	wait(&stat);
	signal(SIGCHLD,childwait);
}

bool startsWith(char *pre, char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

//ornek
//strncpy(buf,ctime(&now),BUF_SIZE);
void servecommand(char *buf, int *indexes, char *messages, int *currentIndex) {
	if(startsWith("SEND",buf)) 
	{
		// bunde her yeni mesajda follow edilen kelime varmı diye kontrol edilip diğer
		// agent lar uyarılmalı
		int baslangicIndex = indexes[(*currentIndex)];
		int sonIndex = baslangicIndex + strlen(buf) - 1;
		indexes[(*currentIndex)+1] = sonIndex;

		int j = 5;

		for(int i = baslangicIndex; i<sonIndex; i ++){
			messages[i] = buf[j];
			j++;
		}

		*currentIndex += 1;
	}
	else if(startsWith("LAST",buf))
	{

	}
	else
	{
		strncpy(buf,"NOT SUPPORTED YET",BUF_SIZE);
	}

	printf("\n");
	for(int k = 0; k<10; k ++){
		printf("%d -",indexes[k]);
	}
	printf("\n");

	char subMes[100];
	strncpy(subMes,messages,100);
	printf("%s",subMes);

	printf("\n");
}
	
void agent(int sharedStartIndexKey, int sharedMessagesKey, int sharedCurrentIndex, int sockfd, int maxnmess, int maxtotmesslen){
	char buf[BUF_SIZE];
	int nread;

	int *indexes;
	char *messages;
	int *currentIndex;

	if ((indexes=(int *)shmat(sharedStartIndexKey,0,0))==NULL) {
		perror("Attaching shared mem");
		exit(-1);
	}

	if ((messages=(char *)shmat(sharedMessagesKey,0,0))==NULL) {
		perror("Attaching shared mem");
		exit(-1);
	}

	if ((currentIndex=(int *)shmat(sharedCurrentIndex,0,0))==NULL) {
		perror("Attaching shared mem");
		exit(-1);
	}

	while (1) {		
		nread=recv(sockfd,buf,BUF_SIZE,0);
    	printf("received: len=%d, content=%s\n",
		            	nread,buf);

		if (nread<0)
			break;
		else if(strcmp(buf, "quit\n") == 0){
			send(sockfd,buf,strlen(buf)+1,0);
			break;
		}

		servecommand(buf, indexes, messages, currentIndex);

		nread=send(sockfd,buf,strlen(buf)+1,0);

		if (nread<0)
			perror("writing:");
		else 			
			printf("wrote: %d bytes, %s\n",nread,buf);
	}
	close(sockfd);
	return;
}

int main(int argc, char *argv[])
{
    char c;
    FILE *fp;
    int fromlen;
    int i,j, s, nread, len,plen,ns;
    struct sockaddr_un saun;
    struct sockaddr_un paun;
    char buf[BUF_SIZE];

	if(argc != 4){
		printf("Unknown usage. Usage : './messboard maxnmess maxtotmesslen address' \n");
		return 0;
	}

	char* address = argv[3];
	int maxnmess = strtol(argv[1], NULL, 10);
	int maxtotmesslen = strtol(argv[2], NULL, 10);

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("server: socket");
        exit(1);
    }

    saun.sun_family = AF_UNIX;
    strcpy(saun.sun_path, address);

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

	int sharedStartIndexKey, sharedMessagesKey;

	int sharedCurrentIndex;

	if ((sharedStartIndexKey=shmget(IPC_PRIVATE,sizeof(int)*maxnmess,IPC_CREAT|0600))<0) {
		perror("Creating shared mem");
		exit(-1);
	}

	if ((sharedMessagesKey=shmget(IPC_PRIVATE,sizeof(char)*maxtotmesslen,IPC_CREAT|0600))<0) {
		perror("Creating shared mem");
		exit(-1);
	}

	if ((sharedCurrentIndex=shmget(IPC_PRIVATE,sizeof(int),IPC_CREAT|0600))<0) {
		perror("Creating shared mem");
		exit(-1);
	}

    while ((ns=accept(s,(struct sockaddr *)&paun,&plen))>=0) {
	    printf("accepted peer address: len=%d, fam=%d, path=%s\n",
			            plen,paun.sun_family, paun.sun_path);

		if (fork())
		{
			continue;
		}
		else
		{
			agent(sharedStartIndexKey,sharedMessagesKey,sharedCurrentIndex,ns,maxnmess,maxtotmesslen);
			break;
		}
	}
	printf("agent closed \n");
	wait(NULL);
    close(s);

    exit(0);
}
