CFLAGS=-I../libalfs -I..
LDLIBS=-L../libalfs -lalfs

LIBS=$(patsubst %.c,%.so,$(wildcard *.c))

all: $(LIBS)

include ../inc.mak
