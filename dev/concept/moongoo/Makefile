CC=gcc
CFLAGS=-std=c99 -D_BSD_SOURCE -I. 

CFLAGS+=`xml2-config --cflags`
LDLIBS+=`xml2-config --libs`

CFLAGS+=-Wall -Werror -g3

BIN=moongoo
OBJS=alfs.o book.o util.o repl.o syn.o

BOOK=~/projects/alfs/book/index.xml
#BOOK=~/projects/hive/unstable/profile.xml

.PHONY: all clean compile run memcheck debug todo upload bug

all: run

clean:
	rm -f $(BIN) $(OBJS)
	
compile: $(BIN)

run: compile
	./$(BIN) $(BOOK)

memcheck: compile
	valgrind -q --tool=memcheck $(BIN) $(BOOK)

debug: compile
	@echo "Book: $(BOOK)"
	gdb ./$(BIN)

todo:
	@grep "TODO:" *.c *.h|cut -d" " -f3-|sort

upload: clean
	@release_snap
	@scp moongoo-*.tar.bz2 l14h.org:~/public_html/nALFS2/snap/
	@rm -f moongoo-*.tar.bz2

bug: 
	@grep "BUG:" *.c *.h|cut -d" " -f3-|sort

$(BIN): $(OBJS)
alfs.o: alfs.h
util.o: util.h
book.o: book.h
repl.o: repl.h
