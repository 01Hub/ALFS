CC=gcc
CFLAGS=-std=c99 -D_BSD_SOURCE -I. 

CFLAGS+=`xml2-config --cflags`
LDLIBS+=`xml2-config --libs`

CFLAGS+=-Wall -Werror -g3

BIN=moongoo
OBJS=alfs.o book.o util.o repl.o syn.o nalfs.o ass.o devel.o build.o

BOOK=~/projects/alfs/book/index.xml
#BOOK=~/projects/alfs/lfs-profile/LFS.xml
#BOOK=~/projects/alfs/ass/index.xml
#BOOK=~/projects/hive/unstable/profile.xml

OPTS=

.PHONY: all clean compile run memcheck debug todo

all: run

clean:
	rm -f $(BIN) $(OBJS)
	
compile: $(BIN)

run: compile
	./$(BIN) $(OPTS) $(BOOK)

memcheck: compile
	valgrind -q --tool=memcheck $(BIN) $(OPTS) $(BOOK)

debug: compile
	@echo "Book: $(OPTS) $(BOOK)"
	gdb ./$(BIN)

todo:
	@grep "TODO:" *.c *.h|cut -d" " -f3-|sort

$(BIN): $(OBJS)
alfs.o: alfs.h
util.o: util.h
book.o: book.h
repl.o: repl.h
nalfs.o: nalfs.h
ass.o: ass.h
devel.o: devel.h
build.o: build.h
