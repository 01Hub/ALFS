CFLAGS=-I. -Ilibalfs

BIN=moongoo
OBJS=build.o moongoo.o plugin.o url.o

ifeq ($(OPTS),)
OPTS=-q -s blfs
#OPTS+=~/projects/alfs/book/index.xml
#OPTS+=~/projects/alfs/lfs-profile/LFS.xml
#OPTS+=~/projects/alfs/ass/index.xml
#OPTS+=~/projects/hive/unstable/profile.xml
OPTS+=~/projects/alfs/blfs/index.xml
#OPTS+=~/projects/alfs/hlfs/index.xml
#OPTS+=~/projects/alfs/book-rng/index.xml
endif

SUBDIRS=libalfs syntax test

.PHONY: all

all: all-subdirs run

include inc.mak

$(BIN): $(OBJS) libalfs/libalfs.a

memcheck: $(BIN)
	valgrind --tool=memcheck ./$(BIN) $(OPTS)

run: $(BIN)
	./$(BIN) $(OPTS)

build.o: build.h
moongoo.o: moongoo.h
plugin.o: plugin.h
url.o: url.h
