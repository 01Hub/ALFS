LIB=libcomm.a
OBJS=libcomm.o reader.o writer.o ssl.o secret.o remote.o

CFLAGS=-I../libalfs -g3

.PHONY: all

all: $(LIB)

include ../libalfs/inc.mak

$(LIB): $(OBJS)
	ar crv $@ $^
	ranlib $@
