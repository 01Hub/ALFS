CFLAGS=-I.

LIB=libalfs.a
OBJS=alfs.o devel.o gen.o net.o parse.o repl.o util.o

all: $(LIB)

include ../inc.mak

$(LIB): $(OBJS)
	ar crv $@ $^
	ranlib $@

alfs.o: alfs.h
devel.o: devel.h
gen.o: gen.h
net.o: net.h
parse.o: parse.h
repl.o: repl.h
util.o: util.h
