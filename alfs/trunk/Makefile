CFLAGS=-g3 -Ilibalfs -Ilibcomm -D_REENTRANT
LDLIBS=-lpthread

BIN1=test
OBJS1=build.o test.o plugin.o url.o

BIN2=alfscl
OBJS2=alfscl.o plugin.o build.o

BIN3=alfsd
OBJS3=alfsd.o plugin.o build.o

CLEANFILES=$(BIN1) $(BIN2) $(BIN3) gdb.exe

ifeq ($(OPTS),)
OPTS=-s ass
#OPTS+=~/stuff/LFS/SVN/lfs/index.xml
#OPTS+=~/projects/alfs/books/multi-arch/index.xml
#OPTS+=~/projects/alfs/books/book/index.xml
#OPTS+=~/projects/alfs/books/lfs-profile/LFS.xml
OPTS+=~/projects/alfs/books/ass/index.xml
#OPTS+=~/projects/hive/unstable/profile.xml
#OPTS+=~/projects/alfs/books/blfs/index.xml
#OPTS+=~/projects/alfs/books/hlfs/index.xml
#OPTS+=~/projects/alfs/books/book-rng/index.xml
#OPTS+=~/projects/alfs/books/refbuild/xml/x86-reference-build.xml
endif

SUBDIRS=libalfs syntax output libcomm

.PHONY: all memcheck run

all: all-subdirs run

include libalfs/inc.mak

$(BIN1): $(OBJS1) libalfs/libalfs.a

LDLIBS+=-lnc_sock

$(BIN2): $(OBJS2) libcomm/libcomm.a libalfs/libalfs.a
$(BIN3): $(OBJS3) libcomm/libcomm.a libalfs/libalfs.a

memcheck: $(BIN1)
	valgrind --tool=memcheck ./$(BIN1) $(OPTS)

run: $(BIN1) $(BIN2) $(BIN3)
	./$(BIN2) $(OPTS)
