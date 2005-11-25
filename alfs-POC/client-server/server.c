#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

int
main ()
{

	#define MAXLINE 4096
	#define LISTENQ 5
	#define SA struct sockaddr

	int listenfd, connfd;
	struct sockaddr_in servaddr;
	char buff[MAXLINE];
	
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(1234);
	
	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);
	
	for ( ; ; ) {
		connfd = accept(listenfd, (SA *) NULL, NULL);
		
		snprintf(buff, sizeof(buff), "This is a test\r\n");
		write(connfd, buff, strlen(buff));
	
		close(connfd);
	}
}
