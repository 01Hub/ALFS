CFLAGS=-I. -Ilibalfs
LDLIBS=-Llibalfs -lalfs

BIN=moongoo
OBJS=moongoo.o build.o plugin.o

ifeq ($(OPTS),)
OPTS=
#OPTS+=~/projects/alfs/book/index.xml
#OPTS+=~/projects/alfs/lfs-profile/LFS.xml
OPTS+=~/projects/alfs/ass/index.xml
#OPTS+=~/projects/hive/unstable/profile.xml
endif

SUBDIRS=libalfs syntax

.PHONY: all

all: all-subdirs run

include inc.mak

$(BIN): $(OBJS)

run: $(BIN)
	./$(BIN) $(OPTS)
