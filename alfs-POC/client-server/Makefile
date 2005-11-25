all: client server

client: client.c
	gcc -W -Wall -Werror -pedantic -std=gnu99 -D_GNU_SOURCE client.c -o client

server: server.c
	gcc -W -Wall -Werror -pedantic -std=gnu99 -D_GNU_SOURCE server.c -o server

clean:
	rm client server
