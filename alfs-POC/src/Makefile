all: alfs alfsd

alf%:
	gcc -W -Wall -Werror -pedantic -std=gnu99 -D_GNU_SOURCE $@.c -o $@

clean:
	rm alfs alfsd
