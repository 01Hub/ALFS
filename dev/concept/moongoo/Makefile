CFLAGS=-I. -Ilibalfs

BIN=moongoo
OBJS=build.o moongoo.o plugin.o url.o

ifeq ($(OPTS),)
OPTS=-q
OPTS+=~/projects/alfs/book/index.xml
#OPTS+=~/projects/alfs/lfs-profile/LFS.xml
#OPTS+=~/projects/alfs/ass/index.xml
#OPTS+=~/projects/hive/unstable/profile.xml
endif

SUBDIRS=libalfs syntax

.PHONY: all

all: all-subdirs run

include inc.mak

$(BIN): $(OBJS) libalfs/libalfs.a

run: $(BIN)
	./$(BIN) $(OPTS)

build.o: build.h
moongoo.o: moongoo.h
plugin.o: plugin.h
url.o: url.h
