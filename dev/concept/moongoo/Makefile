CFLAGS=-I. -Ilibalfs

BIN=moongoo
OBJS=build.o main.o plugin.o url.o

ifeq ($(OPTS),)
OPTS=-s ass -o ass_out
#OPTS+=~/projects/alfs/book/index.xml
#OPTS+=~/projects/alfs/lfs-profile/LFS.xml
OPTS+=~/projects/alfs/ass/index.xml
#OPTS+=~/projects/hive/unstable/profile.xml
#OPTS+=~/projects/alfs/blfs/index.xml
#OPTS+=~/projects/alfs/hlfs/index.xml
#OPTS+=~/projects/alfs/book-rng/index.xml
#OPTS+=~/projects/alfs/refbuild/xml/x86-reference-build.xml
endif

SUBDIRS=libalfs syntax test output

.PHONY: all

all: all-subdirs run

include inc.mak

$(BIN): $(OBJS) libalfs/libalfs.a

memcheck: $(BIN)
	valgrind --tool=memcheck ./$(BIN) $(OPTS)

run: $(BIN)
	./$(BIN) $(OPTS)

build.o: build.h
main.o: main.h
plugin.o: plugin.h
url.o: url.h
