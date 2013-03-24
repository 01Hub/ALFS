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

nALFS -S -l -L nalfs.log BLFS.xml

