#!/bin/bash

if [ ! -d /var/log/nALFS ]; then
    mkdir /var/log/nALF
fi
export NALFS_STAMP_DIR=/var/log/nALFS

CURRENTDIR=`pwd`
echo "<!ENTITY blfs-config '$CURRENTDIR/config'>" > files.ent

/usr/bin/nALFS -S -l -L nalfs.log BLFS.xml
