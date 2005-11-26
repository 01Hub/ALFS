#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/wait.h>

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


            if (!fork()) { /* this is the child process */
                if (recv(connfd, buff, sizeof(buff), 0) == -1)
                    perror("recv");

		printf("SERVER: Executing %s\n", buff);

		if (system(buff) == -1)
		    perror("system");

		snprintf(buff, 29, "CLIENT: Command successful!\n");
		write(connfd, buff, strlen(buff));
		close(connfd);
                exit(0);
            }

	    close(connfd);

            while(waitpid(-1,NULL,WNOHANG) > 0); /* clean up child processes */

	}
}

