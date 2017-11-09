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
char * servecommand(char *buf, int *indexes, char *messages, int *currentIndex, int maxnmess, int maxtotmesslen, char *out) {
	if(startsWith("SEND",buf)) 
	{
		out = (char *) malloc(sizeof(char) * 100);
		strcpy(buf, buf + 5);

		int baslangicIndex = indexes[(*currentIndex)%maxnmess];
		int sonIndex = baslangicIndex + strlen(buf);
		
		indexes[((*currentIndex)+1)%maxnmess] = sonIndex;

		strcpy(messages+baslangicIndex, buf);

		*currentIndex += 1;
		strcpy(out, "<ok>");
	}
	else if(startsWith("LAST",buf))
	{
		strcpy(buf, buf + 5);
		int lastCount = atoi(buf);
		if(lastCount == 0) lastCount = maxnmess;

		int sonIndex = indexes[(*currentIndex)%maxnmess];
		int baslangicIndex = indexes[((*currentIndex) - lastCount)%maxnmess];

		printf("son index = %d\n", sonIndex);
		printf("baslangic index = %d\n", baslangicIndex);

		if(sonIndex > baslangicIndex){
			out = (char *) malloc(sizeof(char) * (sonIndex - baslangicIndex));
			strncpy(buf, messages+baslangicIndex, sonIndex - baslangicIndex);
			printf("-------------------\n");
			printf("%s",buf);
			printf("-------------------\n");
		}
		else{
			printf(" ELSEEEEEEEEEEEEEEEEE\n");
			char * firstPart = (char *) malloc(sizeof(char) *(maxtotmesslen - baslangicIndex + 1));
			strncpy(firstPart, messages+baslangicIndex, maxtotmesslen - baslangicIndex);

			char * secondPart = (char *) malloc(sizeof(char) *(sonIndex+1));
			strncpy(secondPart, messages, sonIndex);
			
			out = (char *) malloc(1 + strlen(firstPart)+ strlen(secondPart) );
			strcpy(out, firstPart);
			strcat(out, secondPart);
			free(firstPart);
			free(secondPart);
		}
		printf("LAST COUNT = %d \n",lastCount);
	}
	else
	{
		printf("************\n");
		out = (char *) malloc(sizeof(char) * 100);
		printf("************\n");
		strncpy(out,"NOT SUPPORTED YET",BUF_SIZE);
		printf("************\n");
	}

	printf("-------------------\n");
	for(int k = 0; k<10; k ++){
		printf("%d -",indexes[k]);
	}
	printf("\n-------------------\n");

/*
	char * subMes = (char *) malloc(sizeof(char) * 100);
	printf("-------------------\n");
	strncpy(subMes, messages,100);
	printf("%s",subMes);
	printf("-------------------\n");
*/	
	return out;
}
	
void agent(int sharedStartIndexKey, int sharedMessagesKey, int sharedCurrentIndex, int sockfd, int maxnmess, int maxtotmesslen){
	char * buf;
	buf = (char *) malloc(BUF_SIZE);
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
		memset(buf, 0, sizeof buf);
		nread=recv(sockfd,buf,BUF_SIZE,0);
    	printf("received: len=%d, content=%s\n",
		            	nread,buf);

		if (nread<0)
			break;
		else if(strcmp(buf, "quit\n") == 0){
			send(sockfd,buf,strlen(buf)+1,0);
			break;
		}

		char * out;

		servecommand(buf, indexes, messages, currentIndex, maxnmess, maxtotmesslen, out);

		free(out);

		nread=send(sockfd,out,strlen(out)+1,0);

		if (nread<0)
			perror("writing:");
		else 			
			printf("wrote: %d bytes, %s\n",nread,out);
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
