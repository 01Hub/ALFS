CFLAGS=-I.

LIB=libalfs.a
OBJS=alfs.o devel.o gen.o net.o repl.o util.o

all: $(LIB)

include ../inc.mak

$(LIB): $(OBJS)
	ar crv $@ $^
	ranlib $@

alfs.o: alfs.h
devel.o: devel.h
gen.o: gen.h
net.o: net.h
repl.o: repl.h
util.o: util.h
