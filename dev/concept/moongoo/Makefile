CFLAGS=-I. -Ilibalfs

BIN=moongoo
OBJS=moongoo.o build.o plugin.o

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
