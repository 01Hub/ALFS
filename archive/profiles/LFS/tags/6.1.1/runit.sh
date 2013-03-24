#!/bin/sh

if [ $(id -u) -ne 0 ]; then
   echo "Attention: this profile should be run as root"
   exit 3
fi

if [ ! -e config ]; then
  echo "Attention: You must create a config directory (or link)"
fi

CURRENTDIR=`pwd`
echo "<!ENTITY lfs-profile '${CURRENTDIR}'>" > config/profile.ent

if [ -z "${1}" ]; then
  SKELETON=skeleton
else
  SKELETON=${1}
fi
echo "<!ENTITY skeleton '${SKELETON}'>" >> config/profile.ent

umask 022
nALFS -l -L nalfs.log LFS.xml
