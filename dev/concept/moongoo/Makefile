BIN=moongoo
OBJS=alfs.o util.o repl.o devel.o build.o plugin.o

ifeq ($(OPTS),)
OPTS=
OPTS+=~/projects/alfs/book/index.xml
#OPTS+=~/projects/alfs/lfs-profile/LFS.xml
#OPTS+=~/projects/alfs/ass/index.xml
#OPTS+=~/projects/hive/unstable/profile.xml
endif

.PHONY: all

all: run

include inc.mak
