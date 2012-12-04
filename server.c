#define BUFLEN 1024
#define PORT 123

#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

char buffer[BUFLEN];
int sockfd;

typedef struct infoForTasker
{
	int fd;
	int sockfd;
} TASKERINFO;

int authentication(char *login, char *password)
{	
	FILE *fpread;
	if((fpread = fopen("passwords.txt", "r")) == NULL)
	{
		perror("fopen");
		exit(1);
	}
	int ptemp = getc(fpread);
	int counter;
	while(1)
	{
		counter = 0;
		do
		{
			if((ptemp)!=*(login+counter))
				break;
			counter++;
		} while((ptemp = getc(fpread)) != ' ');
		counter = 0;
		if(ptemp == ' ')
		{
			while((ptemp = getc(fpread)) != '\n' && (ptemp)==*(password +counter))
			{
				counter++;
			}
			if((ptemp)=='\n' && counter == strlen(password))
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		while((ptemp = getc(fpread)) != '\n')
			;
		if((ptemp = getc(fpread)) == EOF)
		{
			return -1;
		}
	}
	return -1;
}
/*
int readMessage(char *str)
{
	if(!strcmp("date", str))
	{
		time_t seconds = time(NULL);
		strftime(str, BUFLEN, "%d %B %Y", gmtime(&seconds));
		return 0;
	}
	if(!strcmp("time", str))
	{
		time_t seconds = time(NULL);
		strftime(str, BUFLEN, "%H:%M", gmtime(&seconds));
		return 0;
	}
	if(!strcmp("close", str))
	{
		return -1;
	}
	strcpy(str, "wrong command");
	return 0;
}*/

int readMessage(char *cmd)
{
	FILE *ptr;
	int size = 0;

	if(!strcmp("close", cmd))
	{
		return -1;
	}

	if ((ptr = popen(cmd, "r")) != NULL)
	{
		size = fread(cmd, sizeof(char), BUFLEN, ptr);
		cmd[size] = '\0';
	}
	else
	{
		perror("popen");
		pclose(ptr);
		return -1;
	}
	pclose(ptr);
	return size;
}

void handing(int sockfd)
{
	int resOfOper;
	char buffer[BUFLEN];

	//read login
	if(read(sockfd, buffer, BUFLEN-1)<0)
	{
		perror("read login");
		return;
	}
	char login[strlen(buffer)];
	strcpy(login, buffer);

	char *message = "Enter your password";
	if(write(sockfd, message, strlen(message)+1)<0)
	{
		perror("write message about password");
		return;
	}
	//read password
	if(read(sockfd, buffer, BUFLEN-1)<0)
	{
		perror("read password");
		return;
	}
	//change login and password
	if(authentication(login, buffer)<0)
	{
		message = "Invalid login or password. Davay dosvidaniya.";
		write(sockfd, message, strlen(message)+1);
		return;
	}
	message = "Verification complited\n";
	if(write(sockfd, message, strlen(message)+1)<0)
	{
		perror("write verification");
		return;
	}

	printf("[%s] joined\n", login);

	int result;
	//managment
	while((resOfOper = read(sockfd, buffer, BUFLEN-1))>0)
	{
		result = readMessage(buffer);
		if(result < 0)
		{
			printf("[%s] disconnected\n", login);
			return;
		}
		if(result == 0)
		{
			buffer[0] = '\0';
		}
		if(write(sockfd, buffer, strlen(buffer)+1)<0)
		{
			perror("write");
			return;
		}
	}
   
	if(resOfOper < 0)
	{
		perror("read");
		return;
	}
}

void threadFunc(void *pfd)
{
	//fpos_t pos;

	int* fd = (int*)(pfd);
	int socket;

	while(1)
	{
		read(*fd, &socket, 4);

		if(socket < 0)
		{
			break;
		}
		handing(socket);
		close(socket);
	}
}

void tasker(void* pInfo)
{
	TASKERINFO *info = (TASKERINFO*)(pInfo);
	int fd = info->fd;
	int sockfd = info->sockfd;


	struct sockaddr_in clientAddr;
	int sizeOfStructAdrr = sizeof(clientAddr);
	int newsockfd;

	printf("tasker loaded\n");

	while(1)
	{
		if((newsockfd = accept(sockfd, (struct sockaddr *) &clientAddr, &sizeOfStructAdrr))<0)
		{
			perror("accept");
			close(newsockfd);
		}
		printf("\nclient connected %s\n", inet_ntoa(clientAddr.sin_addr));
		write(fd, &newsockfd, 4);
	}

	close(newsockfd);
}

int canselThreads(int countOfThreads, pthread_t threads[])
{
	int result = 0;
	int i;
	for(i=countOfThreads; i<0; --i)
	{
		if(pthread_cancel(threads[i])!=0)
		{
			result++;
		}
	}
	return result;
}

int main(int argc, char *argv[])
{
	if(argc<3)
	{
		printf("arguments not found\n");
		exit(1);
	}
	int port = atoi(argv[1]); //3425
	int countOfThreads = atoi(argv[2]) + 1;

	int fd[2];
	pipe(fd);

	pthread_t threads[countOfThreads];
	int *sockets[countOfThreads];

	int sockfd, newsockfd;
	char buffer[BUFLEN];
	struct sockaddr_in servaddr;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0)
	{
		perror("socket");
		exit(1);
	}
	printf("Socked created\n");
    
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int sizeOfStructAdrr = sizeof(servaddr);

	if(bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	{
		perror("bind");
		close(sockfd);
		exit(2);
	}
	printf("Binding complited\n");

	//creating threads
	int i;
	for(i=1; i<countOfThreads; i++)
	{
		pthread_create(&threads[i], NULL, threadFunc, (void*)(&fd[0]));
	}

	if(listen(sockfd, countOfThreads) < 0){
		perror("listen");
		close(sockfd);
		exit(1);
	}
	printf("end listen\n");


	TASKERINFO *info = (TASKERINFO*)malloc(sizeof(TASKERINFO));;
	
	info->fd = fd[1];
	info->sockfd = sockfd;
	
	//tasker
	pthread_create(&threads[0], NULL, tasker, (void*)(info));

	int serverIsWorking = 1;
	char command[BUFLEN];
	while(serverIsWorking>0)
	{
		fgets(command, BUFLEN, stdin);
		command[strlen(command)-1] = '\0';

		if(!strcmp("quit", command))
		{
			serverIsWorking = 0;
			int resultClosing = canselThreads(countOfThreads, threads);
			if(resultClosing >0)
			{
				printf("failed to close %d threads\nserver is closing\n", resultClosing);
				close(sockfd);
				exit(1);
			}
			else
			{
				printf("all threads canceled. server is closing\n");
			}
		}
		else
		{
			printf("wrong command\n");
		}
	}

	close(sockfd);
	return 0;    
}
