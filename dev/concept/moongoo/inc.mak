CC=gcc
CFLAGS+=-std=c99 -D_BSD_SOURCE -D_GNU_SOURCE
CFLAGS+=-Wall -Werror -g3

CFLAGS+=`xml2-config --cflags` `curl-config --cflags`
LDLIBS+=`xml2-config --libs` `curl-config --libs` -ldl

.PHONY: clean todo all-subdirs clean-subdirs sloccount

all-subdirs:
	@for dir in $(SUBDIRS) ; do \
		(cd $$dir && $(MAKE) $(MARGS) all) || exit 1; \
	done

clean-subdirs:
	@for dir in $(SUBDIRS) ; do \
	    (cd $$dir && $(MAKE) $(MARGS) clean) || exit 1; \
	done

clean: clean-subdirs
	rm -f $(BIN) $(LIB) $(LIBS) *.o
	
todo:
	@find -name '*.c'|xargs grep "TODO:"|cut -d" " -f3-|sort

sloccount:
	@find -name '*.c'|xargs wc -l|tail -n 1|awk '{print $$1}'

%.so: %.o
	$(CC) -shared $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)
