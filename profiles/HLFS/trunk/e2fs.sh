#!/bin/sh
find resize/ debugfs/ e2fsck/ misc/ -type f -name Makefile.in | xargs sed -e 's@\$(ALL_CFLAGS)@& -pie -fpie@g' -i
