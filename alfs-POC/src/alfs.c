#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

int
main (int argc, char **argv)
{

	#define MAXLINE 4096 /* used for buffer size */
	#define SA struct sockaddr /* just a shortcut, saves typing
				      later in the connect() function
				      and prevents that line from wrapping
				   */
	#define PORT 1234
		
	int sockfd, n, len;
	char recvline [MAXLINE+1];
	char input[256];
	struct sockaddr_in servaddr;
	
	if (argc != 2) {
		printf("usage: %s IPaddress\n", argv[0]);
		exit(1);
	}

	/* setup socked descriptor */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/* set structure to zero */
	bzero(&servaddr, sizeof(servaddr));

	/* set to IPv4 */
	servaddr.sin_family = AF_INET;

	/* Use port 1234 */
	servaddr.sin_port = htons(PORT);

	/* convert provided IP address into numeric */
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	/* connect to socket descriptor to port set above*/
	if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) == -1)
	{
		printf("Connection failed!\n");
		exit(1);
	} else {
		printf("Connected to %s\n", argv[1]);
	}

	/* Read input from the user to send to the server */
	printf("Enter a command to send to the server:\n");
	fgets(input, sizeof(input), stdin);
	fflush(stdin);
	len = strlen(input);

	/* Send the input to the server */
	send(sockfd, input, len, 0);

	/* read data from server */	
	n = read(sockfd, recvline, MAXLINE);
	recvline[n] = 0;

	if (fputs(recvline, stdout) == EOF)
		printf("fputs error\n");

	if (n < 0)
		printf("read error\n");
	
	exit(0);
}

