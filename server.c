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
#include <semaphore.h>
#include <fcntl.h>

#define BUF_SIZE 256
#define SNAME "/mysem"

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
	printf("***************************************************************\n");
	for (int i = 0; i < maxtotmesslen; ++i)
	{
		printf("%c - ",messages[i] );
	}
	printf("***************************************************************\n");

	if(startsWith("SEND",buf)) 
	{
		strcpy(buf, buf + 5);

		int baslangicIndex = indexes[*currentIndex];
		int sonIndex = baslangicIndex + strlen(buf)-1;

		if((*currentIndex) == maxnmess || sonIndex > maxtotmesslen){
			baslangicIndex = 0;
			(*currentIndex) = 0;
			sonIndex = strlen(buf)-1;
		}

		int tmpIndex = *currentIndex + 1;
		printf("***************************************************************\n");
		printf("***************************************************************\n");
		printf("%d\n",indexes[tmpIndex]);
		printf("%d\n",sonIndex);
		printf("***************************************************************\n");
		printf("***************************************************************\n");
		if(!indexes[tmpIndex]){
			indexes[tmpIndex] = sonIndex;
		}
		else if(indexes[tmpIndex] == sonIndex){
			;
		}
		else if(indexes[tmpIndex] > sonIndex)
		{
			memset(messages + baslangicIndex, 0, indexes[tmpIndex] - indexes[tmpIndex-1]);
		}
		else if(indexes[tmpIndex] < sonIndex)
		{
			while(indexes[tmpIndex] && indexes[tmpIndex] < baslangicIndex + strlen(buf)){			
				indexes[tmpIndex] = sonIndex;
				tmpIndex += 1;
			}
			for (int i = maxnmess; i > (*currentIndex) + 1; i--)
			{
				if(indexes[i] == sonIndex){
					if(indexes[i+1]){
						indexes[i] = indexes[i+1];
						indexes[i+1] = 0;
					}
					else{
						indexes[i] = 0;
					}
				}
			}
			printf("%c\n",*(messages + baslangicIndex));
			printf("%d\n",baslangicIndex);
			printf("%d\n",(*currentIndex));
			printf("%d\n",indexes[(*currentIndex)+2]);
			printf("%d\n",indexes[(*currentIndex)]);
			if(indexes[(*currentIndex)+2] == 0){
				memset(messages + baslangicIndex, 0, maxtotmesslen - baslangicIndex);
			}
			else{
				memset(messages + baslangicIndex, 0, indexes[(*currentIndex)+2] - indexes[(*currentIndex)]);
			}
		}

		strncpy(messages+baslangicIndex, buf, strlen(buf)-1);

		*currentIndex += 1;
		strcpy(out, "<ok>\n");
	}
	else if(startsWith("LAST",buf))
	{
		strcpy(buf, buf + 5);
		int lastCount = atoi(buf);
		if(lastCount == 0 || lastCount >= maxnmess) lastCount = maxnmess;

		int sonIndex = indexes[*currentIndex];

		int basIndexYeri = (*currentIndex) - lastCount;
		if(basIndexYeri<0) basIndexYeri = maxnmess + basIndexYeri;

		out = (char *) realloc(out,maxtotmesslen);
		memset(out, 0, sizeof out);

		for (int i = 0; i < lastCount; i++)
		{
			strncat(out, messages+indexes[basIndexYeri],indexes[basIndexYeri+1]-indexes[basIndexYeri] );
			if(((*out)+ strlen(out) )){
				printf("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");
				printf("%c\n", (*out)+ strlen(out) );
				printf("%c\n", (*out) );
				printf("%d\n", strlen(out) );
				printf("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");
				strcat(out, "\n");
			}
			basIndexYeri += 1;
			if(basIndexYeri == maxnmess){
				basIndexYeri = 0;
			}
		}
	}
	else if(startsWith("BYE",buf))
	{
		strncpy(out,"BYE\n",BUF_SIZE);
	}
	else
	{
		printf("************\n");
		strncpy(out,"NOT SUPPORTED YET\n",BUF_SIZE);
		printf("************\n");
	}

	printf("-------------------\n");
	for(int k = 0; k<10; k ++){
		printf("%d -",indexes[k]);
	}
	printf("\n-------------------\n");


	printf("///////////////////\n");
	printf("%s",out);
	printf("///////////////////\n");

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


	sem_t *sem = sem_open(SNAME, 0);
	
	while (1) {		
		memset(buf, 0, sizeof buf);
		nread=recv(sockfd,buf,BUF_SIZE,0);
    	printf("received: len=%d, content=%s\n",
		            	nread,buf);

		if (nread<0)
			break;
		

		char * out;
		out = (char *) malloc(sizeof(char) * BUF_SIZE);
		sem_wait(sem);
		servecommand(buf, indexes, messages, currentIndex, maxnmess, maxtotmesslen, out);
		sem_post(sem);

		nread=send(sockfd,out,strlen(out)+1,0);
		if (nread<0){
			perror("writing:");
		}
		else if(strcmp(out, "BYE\n") == 0)
		{
			break;
		}
		else{
			printf("wrote: %d bytes, %s\n",nread,out);
		} 			
	}
	close(sockfd);
	return;
}

int main(int argc, char *argv[])
{
    int s, len,plen,ns;
    struct sockaddr_un saun;
    struct sockaddr_un paun;


	if(argc != 4){
		printf("Unknown usage. Usage : './messboard maxnmess maxtotmesslen address' \n");
		return 0;
	}

	char* address = argv[3];
	int maxnmess = strtol(argv[1], NULL, 10);
	int maxtotmesslen = strtol(argv[2], NULL, 10);

	sem_t *sem = sem_open(SNAME, O_CREAT, 0644, 3); /* Initial value is 3. */
	sem_init(sem, 0, 1);

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

    close(s);

    exit(0);
}
