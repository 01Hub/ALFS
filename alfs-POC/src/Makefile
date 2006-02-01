CC = gcc
FLAGS = -Wall -Werror -pedantic -O2 -pipe -std=gnu99 -D_GNU_SOURCE
BINARIES = parser alfs alfsd

# List 'clean' as a dependency for now so that it will always clean
# before it builds.
all: clean $(BINARIES)

alf%:
	$(CC) $(FLAGS) $@.c -o $@

parser: parser.c
	$(CC) $(FLAGS) $@.c -o $@

clean:
	-rm -f $(BINARIES)

.PHONY: clean
