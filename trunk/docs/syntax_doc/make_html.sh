
export SGML_CATALOG_FILES=/usr/share/docbook/docbook.cat:/usr/share/dsssl/docbook/catalog:/usr/share/dsssl/openjade/catalog


rm -rf html &&
mkdir html &&
cd html &&
openjade -t xml \
	-d /usr/share/dsssl/docbook/html/alfs.dsl \
	/usr/share/dsssl/docbook/dtds/decls/xml.dcl \
	../index.xml

echo "Generation of ALFS DTD documentation completed"
