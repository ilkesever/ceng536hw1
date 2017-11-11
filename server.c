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



int *indexes;
char *messages;
int *currentIndex;
int maxnmess;
int maxtotmesslen;
int mysocket;
char **follow;

bool startsWith(char *pre, char *str)
{
    return strncmp(pre, str, strlen(pre)) == 0;
}

//ornek
//strncpy(buf,ctime(&now),BUF_SIZE);
char * servecommand(char *input,  char *out) {
	input[strlen(input) - 1] = 0;
	if(startsWith("SEND ",input)) 
	{
		input = input + 5;

		int baslangicIndex = indexes[*currentIndex];
		int sonIndex = baslangicIndex + strlen(input);

		/*
		printf("------------------------------\n");
		printf("%s\n",input );
		printf("%d\n",strlen(input));
		printf("------------------------------\n");
		*/

		if((*currentIndex) == maxnmess || sonIndex > maxtotmesslen){
			baslangicIndex = 0;
			(*currentIndex) = 0;
			sonIndex = strlen(input);
		}

		int tmpIndex = *currentIndex + 1;
		printf("***************************************************************\n");
		printf("%d\n",indexes[tmpIndex]);
		printf("%d\n",sonIndex);
		printf("***************************************************************\n");
		if(!indexes[tmpIndex]){
			printf("Case 1\n");
			indexes[tmpIndex] = sonIndex;
		}
		else if(indexes[tmpIndex] == sonIndex){
			printf("Case 2\n");
			;
		}
		else if(indexes[tmpIndex] > sonIndex)
		{
			printf("Case 3\n");
			memset(messages + baslangicIndex, 0, indexes[tmpIndex] - indexes[tmpIndex-1]);
		}
		else if(indexes[tmpIndex] < sonIndex)
		{
			printf("Case 4\n");
			while(indexes[tmpIndex] && indexes[tmpIndex] < sonIndex){			
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
			if(indexes[(*currentIndex)+2] == 0 || indexes[(*currentIndex)+2] - indexes[(*currentIndex)] < 0){
				memset(messages + baslangicIndex, 0, maxtotmesslen - baslangicIndex);
			}
			else{
				memset(messages + baslangicIndex, 0, indexes[(*currentIndex)+2] - indexes[(*currentIndex)]);
			}
		}

		strncpy(messages+baslangicIndex, input, strlen(input));

		*currentIndex += 1;
		strcpy(out, "<ok>\n");
	}
	else if(startsWith("LAST",input))
	{
		input = input + 5;
		int lastCount = atoi(input);
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
				//printf("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");
				//printf("%c\n", (*out)+ strlen(out) );
				//printf("%c\n", (*out) );
				//printf("%d\n", strlen(out) );
				//printf("OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO\n");
				strcat(out, "\n");
			}
			basIndexYeri += 1;
			if(basIndexYeri == maxnmess){
				basIndexYeri = 0;
			}
		}
	}
	else if(startsWith("FOLLOW ",input))
	{
		printf("1--------------------\n");

		input = input + 7;

		for (int i = 0; i < 10000; ++i)
		{
			if(follow[i] != NULL && strcmp(follow[i], input) == 0){
				strncpy(out,"You are already following\n",BUF_SIZE);
				return out;
			}
		}

		for (int i = 0; i < 10000; ++i)
		{
			if(follow[i] == NULL){
				follow[i] = malloc (sizeof (char) * sizeof(input));
				strncpy(follow[i],input,strlen(input));
				strncpy(out,"<ok>\n",BUF_SIZE);
				break;
			}
		}
		printf("2--------------------\n");
		for (int i = 0; i < 10000; ++i)
		{
			if(follow[i] != NULL){
				printf("%s\n",follow[i]);
			}
		}
		printf("3--------------------\n");
	}
	else if(startsWith("UNFOLLOW ",input))
	{
		printf("1--------------------\n");

		input = input + 9;

		for (int i = 0; i < 10000; ++i)
		{
			if(follow[i] != NULL && strcmp(follow[i], input) == 0){
				follow[i] = NULL;
				strncpy(out,"<ok>\n",BUF_SIZE);
			}
		}

		printf("2--------------------\n");
		for (int i = 0; i < 10000; ++i)
		{
			if(follow[i] != NULL){
				printf("%s\n",follow[i]);
			}
		}
		printf("3--------------------\n");
	}
	else if(startsWith("FOLLOWING",input))
	{
		printf("1--------------------\n");

		input = input + 10;

		for (int i = 0; i < 10000; ++i)
		{
			if(follow[i] != NULL){
				strcat(out,follow[i]);
				strcat(out,"\n");
			}
		}
	}
	else
	{
		strncpy(out,"NOT SUPPORTED YET\n",BUF_SIZE);
	}

	printf("-------------------\n");
	for(int k = 0; k<10; k ++){
		printf("%d -",indexes[k]);
	}
	printf("\n-------------------\n");

	printf("***************************************************************\n");
	for (int i = 0; i < maxtotmesslen; ++i)
	{
		printf("%c - ",messages[i] );
	}
	printf("***************************************************************\n");

	return out;
}
	
void agent(int sockfd){
	follow = malloc (sizeof (char *) * 10000);
	mysocket = sockfd;
	char * buf;
	buf = (char *) malloc(BUF_SIZE);
	
	int nread;

	sem_t *sem = sem_open(SNAME, 0);
	
	while (1) 	{		
		memset(buf, 0, BUF_SIZE);
		nread=recv(mysocket,buf,BUF_SIZE,0);
    	printf("received: len=%d, content=%s\n",
		            	nread,buf);

		if (nread<0)
			break;
		else if(strcmp(buf, "BYE\n") == 0)
		{
			break;
		}

		char * out;
		out = (char *) malloc(sizeof(char) * BUF_SIZE);
		sem_wait(sem);
		servecommand(buf, out);
		sem_post(sem);
		
		nread=send(mysocket,out,strlen(out)+1,0);
		if (nread<0){
			perror("writing:");
		}
		else{
			printf("wrote: %d bytes, %s\n",nread,out);
		} 			
	}
	close(mysocket);
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
	maxnmess = strtol(argv[1], NULL, 10);
	maxtotmesslen = strtol(argv[2], NULL, 10);

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

	int sharedStartIndexKey, sharedMessagesKey, sharedCurrentIndex;

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

    while ((ns=accept(s,(struct sockaddr *)&paun,&plen))>=0) {
	    printf("accepted peer address: len=%d, fam=%d, path=%s\n",
			            plen,paun.sun_family, paun.sun_path);

		if (fork())
		{
			continue;
		}
		else
		{
			agent(ns);
			break;
		}
	}

	printf("Agent closed.\n");

    close(s);

    exit(0);
}
