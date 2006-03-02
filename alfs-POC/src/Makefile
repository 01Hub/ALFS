CC = gcc
CXX = g++
CFLAGS = -Wall -Werror -pedantic -O2 -pipe -std=gnu99 -D_GNU_SOURCE
CXXFLAGS = -Wall -Werror -pedantic -pipe -D_GNU_SOURCE
BINARIES = parser alfs alfsd cpp-parser

# List 'clean' as a dependency for now so that it will always clean
# before it builds.
all: clean $(BINARIES)

alf%:
	$(CC) $(CFLAGS) $@.c -o $@

parser: parser.c
	$(CC) $(CFLAGS) $@.c -o $@

cpp-parser: parser.cpp
	$(CXX) $(CXXFLAGS) parser.cpp -o $@

clean:
	-rm -f $(BINARIES)

.PHONY: clean
