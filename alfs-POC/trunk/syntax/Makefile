CFLAGS=-I../libalfs -I..

LIBS=ass.so blfs.so book.so diyl.so nalfs.so syn.so
#LIBS+=sample.so

CLEANFILES=$(LIBS)

all: $(LIBS)

include ../libalfs/inc.mak

ass.so: ../libalfs/libalfs.a
blfs.so: ../libalfs/libalfs.a
book.so: ../libalfs/libalfs.a
diyl.so: ../libalfs/libalfs.a
nalfs.so: ../libalfs/libalfs.a
syn.so: ../libalfs/libalfs.a
