#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{

	#define MAXLINE 4096
	#define SA struct sockaddr
	
	int sockfd, n;
	char recvline [MAXLINE+1];
	struct sockaddr_in servaddr;
	
	if (argc != 2) {
		printf("usage: %s IPaddress\n", argv[0]);
		exit(1);
	}
	
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
		
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(1234);
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	
	connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
	
	while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
		recvline[n] = 0;
		if (fputs(recvline, stdout) == EOF)
			printf("fputs error\n");
	}
	if (n < 0)
		printf("read error\n");
	
	exit(0);
}
