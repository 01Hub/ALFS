cp $1 $1.bak
tidy -config ../tools/tidy.conf $1
sed -i 's/\&amp;/\&/g' $1
rm $1.bak
