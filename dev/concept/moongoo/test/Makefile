CC=gcc

CFLAGS=-I. -I../libalfs

TESTS=tst_libtar tst_md5 tst_sha1 tst_squeeze

.PHONY: all

all: run

run: $(TESTS)
	@for test in $(TESTS); do \
		(echo "Running $$test..." && exec ./$$test) || exit $$?; \
	done

include ../inc.mak

tst_libtar: tst.o ../libalfs/libalfs.a
tst_md5: tst.o ../libalfs/libalfs.a
tst_sha1: tst.o ../libalfs/libalfs.a
tst_squeeze: tst.o ../libalfs/libalfs.a
