CFLAGS=-I.

LIB=libalfs.a
OBJS=alfs.o devel.o repl.o util.o

all: $(LIB)

include ../inc.mak

$(LIB): $(OBJS)
	ar crv $@ $^
	ranlib $@

alfs.o: alfs.h
devel.o: devel.h
repl.o: repl.h
util.o: util.h
