LIB=libalfs.a
OBJS=alfs.o devel.o option.o parse.o repl.o

CLEANDIRS=doc

all: $(LIB)

include inc.mak

$(LIB): $(OBJS)
	ar crv $@ $^
	ranlib $@
