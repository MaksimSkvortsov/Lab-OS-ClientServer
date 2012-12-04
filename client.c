#define IP "127.0.0.1"
#define PORT 123
#define BUFLEN 1024

#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

void newDialog(int sockfd)
{
	int resOfOper;
	char buffer[BUFLEN];

	printf("Write your login:\n");
	fgets(buffer,BUFLEN,stdin);
	buffer[strlen(buffer)-1] = '\0';

	while((resOfOper = write(sockfd, buffer, strlen(buffer)+1))>0)
	{
		if(read(sockfd, buffer, BUFLEN-1)<0)
		{
			perror("write");
			close(sockfd);
			return;
		}
		if(!strcmp(buffer, "close"))
		{
			return;
		}
		printf("%s\n", buffer);
		fgets(buffer, BUFLEN, stdin);
		buffer[strlen(buffer)-1] = '\0';
	}
	if(resOfOper < 0)//error
	{
		perror("read");
		return;
	}
}


void dialog(int sockfd)
{
	int resOfOper;
	char buffer[BUFLEN];

	printf("client ready for reading\n");

	while((resOfOper = read(sockfd, buffer, BUFLEN-1))>0)
	{
		printf("server: %s\n", buffer);
		fgets(buffer, BUFLEN, stdin);

		if(write(sockfd, buffer, strlen(buffer)+1)<0)
		{
			perror("write");
			close(sockfd);
			exit(1);
		}
	printf("buffer = [%s]. size = %d\n", buffer, sizeof(buffer));
	
		if(buffer == "close")
		{
			return;
		}
	}
   
	if(resOfOper < 0)
	{
		perror("read");
		close(sockfd);
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	int sockfd;
	int num;
	char buffer[BUFLEN];
	struct sockaddr_in servaddr;
 
	if(argc<2)
	{
		printf("arguments not found\n");
		exit(1);
	}
	int port = atoi(argv[1]); //3425
   
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0)
	{
		perror("socket");
		exit(1);
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	servaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if(connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0)
	{
		perror("connect");
		close(sockfd);
	
		exit(1);
	}

	printf("Connection complited\n");
	newDialog(sockfd);

	close(sockfd);
	return 0;
}
