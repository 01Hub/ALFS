CFLAGS=-Ilibalfs

BIN=alfs
OBJS=build.o alfs.o plugin.o url.o

ifeq ($(OPTS),)
OPTS=-q
OPTS+=~/projects/alfs/books/multi-arch/index.xml
#OPTS+=~/projects/alfs/books/book/index.xml
#OPTS+=~/projects/alfs/books/lfs-profile/LFS.xml
#OPTS+=~/projects/alfs/books/ass/index.xml
#OPTS+=~/projects/hive/unstable/profile.xml
#OPTS+=~/projects/alfs/books/blfs/index.xml
#OPTS+=~/projects/alfs/books/hlfs/index.xml
#OPTS+=~/projects/alfs/books/book-rng/index.xml
#OPTS+=~/projects/alfs/books/refbuild/xml/x86-reference-build.xml
endif

SUBDIRS=libalfs syntax test output

.PHONY: all memcheck run

all: all-subdirs run

include libalfs/inc.mak

$(BIN): $(OBJS) libalfs/libalfs.a

memcheck: $(BIN)
	valgrind --tool=memcheck ./$(BIN) $(OPTS)

run: $(BIN)
	./$(BIN) $(OPTS)

build.o: build.h
main.o: main.h
plugin.o: plugin.h
url.o: url.h
