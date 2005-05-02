#!/bin/bash

if [ ! -e config ]; then
  echo "Attention: You must create a config directory (or link)."
  exit 3
fi

if [ ! -d /var/log/nALFS ]; then
    mkdir /var/log/nALFS
fi
export NALFS_STAMP_DIR=/var/log/nALFS

CURRENTDIR=`pwd`
echo "<!ENTITY blfs-config '$CURRENTDIR/config'>" > config/files.ent

if [ -e /usr/bin/nALFS ]; then
    /usr/bin/nALFS -S -l -L nalfs.log BLFS.xml
else
    /usr/local/bin/nALFS -S -l -L nalfs.log BLFS.xml
fi

