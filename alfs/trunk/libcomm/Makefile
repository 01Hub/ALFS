LIB=libcomm.a
OBJS=libcomm.o

.PHONY: all

all: $(LIB)

include /usr/share/nc-mk/nc.mak
include /usr/share/nc-mk/libnc.mak

$(LIB): $(OBJS)
	ar crv $@ $^
	ranlib $@
