
xmllint -noent  -loaddtd -valid BLFS.xml 2>&1 | ${PAGER:-more}


#export SP_CHARSET_FIXED=YES
#export SP_ENCODING=XML

#nsgmls -wxml -s  BLFS.xml 

