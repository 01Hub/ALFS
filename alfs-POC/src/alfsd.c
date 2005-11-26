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
	#define PORT 1234
	#define SA struct sockaddr

	int listenfd, connfd, syscmd, pair[2];
	struct sockaddr_in servaddr;
	char buff[MAXLINE];
	
	/* setup socket descriptor */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	/* set structure to zero */	
	bzero(&servaddr, sizeof(servaddr));

	/* set to IPv4 */
	servaddr.sin_family = AF_INET;

	/* listen on any IP address port PORT */
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

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

	    /* Set up pipe from stdout to an empty descriptor */
	    if (pipe(pair) == -1)
		perror("pipe");

	    if (dup2(pair[1],1) == -1)
		perror("dup2");

            if (!fork()) { /* this is the child process */

		/* read a string from the socket into a buffer */
                if (recv(connfd, buff, sizeof(buff), 0) == -1)
                    perror("recv");

		printf("SERVER: Executing %s\n", buff);

		/* execute the string as a system command */
		syscmd = system(buff);

		if (syscmd == 0)
		{
			/* read the empty descriptor (redirected stdout) into the buffer */
			read(pair[0], buff, MAXLINE);
		}

		if (syscmd == 32512)
		    	snprintf(buff, 25, "CLIENT: Command failed!\n");

		if (syscmd == -1)
			perror("system");
	
		write(connfd, buff, strlen(buff));
		close(connfd);
                exit(0);
            }

	    close(connfd);

            while(waitpid(-1,NULL,WNOHANG) > 0); /* clean up child processes */

	}
}

