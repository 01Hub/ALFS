CC=gcc

CFLAGS=-I. -I../libalfs

TESTS=tst_squeeze

.PHONY: all

all: run

run: $(TESTS)
	@for test in $(TESTS); do \
		(echo "Running $$test..." && exec ./$$test) || exit $$?; \
	done

include ../inc.mak

tst_squeeze: tst.o ../libalfs/libalfs.a
