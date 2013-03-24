if [ -z $1 ] ; then
 echo "Usage $0 <arch>"
 exit 1
fi

xmllint --noent --nonet --noout --xinclude --postvalid $1/LFS.xml
